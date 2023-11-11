#include "config/Config.hpp"
#include "server/Server.hpp"
#include "config/LocationBlock.hpp"

// PUBLIC

Server::Server(): _socklen(sizeof(struct sockaddr_in)), _epfd(0), _running(false)
{
	_events = new struct epoll_event[MAX_EVENTS];
}

Server::~Server(void)
{
}

void Server::stop(void)
{
	Client *c;
	std::map<int, Client *>::iterator iter = _clients.begin();

	// close epoll
	if (_epfd)
		close(_epfd);

	// close servers
	for (size_t i = 0; i < _server_fds.size(); i++) {
		close(_server_fds[i]);
	}

	// close clients
	while (iter != _clients.end()) {
		c = iter->second;
		++iter;
		_removeClient(c);
	}
}

static int	create_server_socket(struct sockaddr_in *sockaddr)
{
	const int enable = 1;
    int	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd < 0)
		throw std::runtime_error("failed to create tcp socket");
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		close(fd);
		throw std::runtime_error("failed to set SO_REUSEADDR socket option");
	}
    if (bind(fd, (struct sockaddr *)sockaddr, sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		throw std::runtime_error("failed to bind tcp socket");
	}
    if (listen(fd, LISTEN_BACKLOG) < 0) {
		close(fd);
		throw std::runtime_error("failed to listen");
	}
	return (fd);
}

void Server::serve(void)
{
	int	ret, fd;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
    _epfd = epoll_create1(0);
	if (_epfd < 0)
		throw std::runtime_error("failed to create epoll");
	_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR; // EPOLLERR is default but usefull for readers

	for (size_t i = 0; i < Config::ports.size(); i++) {
		addr.sin_port = htons(Config::ports[i]);
		fd = create_server_socket(&addr);
		_server_fds.push_back(fd);
		_ev.data.fd = fd;
		_ev.data.ptr = &(Config::ports[i]);
		ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &_ev);
		if (0 > ret)
			throw std::runtime_error("failed to add tcp socket to epoll");
	}

	_running = true;
	_eventLoop();
}

// PRIVATE

void Server::_replaceClientEvents(int clientfd, uint32_t op)
{
    _ev.events = op | EPOLLRDHUP | EPOLLERR; // EPOLLERR is default but usefull for readers
    _ev.data.fd = clientfd;
    int ret = epoll_ctl(_epfd, EPOLL_CTL_MOD, clientfd, &_ev);
	if (ret < 0)
		throw std::runtime_error("failed to modify client event");
}

void Server::_acceptClient(int fd, int port)
{
	int cfd, ret;
	Client *c;
	cfd = accept(fd, (struct sockaddr *)&_csin, &_socklen);
	if (cfd < 0) {
		std::cerr << "runtime_error: failed to accept new client" << std::endl;
		return ;
	}
	c = new Client(cfd, port);
	_clients.insert(std::make_pair(cfd, c));
	_ev.events = EPOLLIN;
	_ev.data.fd = cfd;
	_ev.data.ptr = c;
	ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, cfd, &_ev);
	if (ret < 0) {
		close(cfd);
		throw std::runtime_error("failed to add new client to epoll pool");
	}
}

void Server::_removeClient(Client *client)
{
	int ret = epoll_ctl(_epfd, EPOLL_CTL_DEL, client->getFd(), NULL);
	if (ret < 0)
		throw std::runtime_error("failed to add new client to epoll pool");
	_clients.erase(client->getFd());
	delete client;
}

void Server::_handleTimeouts(void)
{
	Client *c;
	std::time_t now = std::time(NULL);
	std::map<int, Client *>::iterator iter;
	iter = _clients.begin();

	while (iter != _clients.end()) {
		if (std::difftime(now, iter->second->getLastActivity()) > CLIENT_TIMEOUT) {
			iter->second->sendRequestTimeout();
			c = iter->second;
			++iter;
			_removeClient(c);
			continue ;
		}
		++iter;
	}
}

void Server::_eventLoop(void)
{
	Client *client;
	int j, nfds;
	std::time_t now;

	j = 0;
    while (true)
    {
        nfds = epoll_wait(_epfd, _events, MAX_EVENTS, EPOLL_MAX_TIMEOUT); // TODO could this bug if wait for extremely long time and double comparaison fail hard?
		if (nfds < 0)
		{
			if (_running == false)
				return ;
			throw std::runtime_error("failed to retrieve events from epoll");
		}
		++j;
		now = std::time(NULL);
        for (int i = 0; i < nfds; i++) {
			try {
				if (_events[i].data.ptr >= &(Config::ports[0]) && _events[i].data.ptr <= &(Config::ports[Config::ports.size() - 1])) { // hacky again
					_acceptClient(_events[i].data.fd, *(int *)_events[i].data.ptr);
					continue ;
				}
				client = (Client *)_events[i].data.ptr;
				client->setLastActivity(now);
				if (_events[i].events & EPOLLIN) {
					if (client->readHandler())
						_removeClient(client);
					if (client->getState() != RECEIVING) // changed state
						_replaceClientEvents(client->getFd(), EPOLLOUT);
				} else if (_events[i].events & EPOLLOUT) {
					try {
						if (client->getState() == PARSING)
							client->parseRequest(); // parse request and set handler
						client->writeHandler();
					} catch (std::runtime_error &e) {
						client->sendInternalServerError();
					}
					if (client->getState() != SENDING) // changed state
						_replaceClientEvents(client->getFd(), EPOLLIN);
				} else {
					// EPOLLERR | EPOLLRDHUP
					_removeClient(client);
				}
			} catch (std::runtime_error &e) {
				std::cerr << "runtime_error: " << e.what() << std::endl;
				_removeClient(client);
			}
        }
		if (j > EPOLL_ROUND_CLEANUP) {
			_handleTimeouts();
			j = 0;
		}
    }
}

