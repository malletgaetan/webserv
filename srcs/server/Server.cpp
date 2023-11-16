#include "config/Config.hpp"
#include "server/Server.hpp"
#include "config/LocationBlock.hpp"
#include <cstring>  // For strerror
#include <cerrno>   // For errno

// PUBLIC

char *Server::_buf = NULL;

Server::Server(): _socklen(sizeof(struct sockaddr_in)), _epfd(0), _running(false)
{
	_events = new struct epoll_event[MAX_EVENTS];
	if (Server::_buf == NULL)
		Server::_buf = new char[SERVER_BUFFER_SIZE];
}

Server::~Server(void)
{
	delete []_events;
	if (Server::_buf != NULL) {
		delete []Server::_buf;
		Server::_buf = NULL;
	}
}

void Server::stop(void)
{
	Client *c;
	std::map<int, Client *>::iterator iter = _clients.begin();

	// close epoll
	if (_epfd)
		close(_epfd);

	// close servers
	for (size_t i = 0; i < Config::ports.size(); i++) {
		close(_servers[i].second);
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
	_ev.events = EPOLLIN;

	_servers = new std::pair<int, int>[Config::ports.size()];

	for (size_t i = 0; i < Config::ports.size(); i++) {
		addr.sin_port = htons(Config::ports[i]);
		fd = create_server_socket(&addr);
		_servers[i].first = fd;
		_servers[i].second = Config::ports[i];
		_ev.data.ptr = _servers + i;
		ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &_ev);
		if (ret != 0)
			throw std::runtime_error("failed to add tcp socket to epoll");
	}

	_running = true;
	_eventLoop();
}

// PRIVATE

void Server::_replaceClientEvents(Client *c, uint32_t op)
{
    _ev.events = op | EPOLLERR | EPOLLRDHUP;
	_ev.data.ptr = c;
    int ret = epoll_ctl(_epfd, EPOLL_CTL_MOD, c->getFd(), &_ev);
	if (ret < 0)
		throw std::runtime_error("failed to modify client event");
}

void Server::_acceptClient(std::pair<int, int> *server)
{
	int cfd, ret;
	Client *c;
	cfd = accept(server->first, (struct sockaddr *)&_csin, &_socklen);
	if (cfd < 0) {
		std::cerr << "runtime_error: failed to accept new client" << std::endl;
		return ;
	}
	c = new Client(cfd, Config::getServers(server->second));
	_clients.insert(std::make_pair(cfd, c));
	_ev.events = EPOLLIN | EPOLLERR | EPOLLRDHUP;
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
		throw std::runtime_error("failed to remove client from epoll pool");
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

// TODO: add a logger
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
				if ((_events[i].data.ptr >= _servers) && (_events[i].data.ptr <= (_servers + Config::ports.size() - 1))) {
					_acceptClient((std::pair<int, int> *)_events[i].data.ptr);
					continue ;
				}
				client = (Client *)_events[i].data.ptr;
				client->setLastActivity(now);
				if (_events[i].events & EPOLLIN) {
					try {
						if (client->readHandler())
							_removeClient(client);
						if (client->getState() != RECEIVING) { // changed state
							client->parseRequest();
							_replaceClientEvents(client, EPOLLOUT);
						}
					} catch (std::runtime_error &e) {
						client->sendInternalServerError();
						_removeClient(client); // reset connection and state in fail case
					}
				} else if (_events[i].events & EPOLLOUT) {
					try {
						client->writeHandler();
					} catch (std::runtime_error &e) {
						client->sendInternalServerError();
						_removeClient(client); // reset connection and state in fail case
					}
					if (client->getState() != SENDING) { // changed state
						_replaceClientEvents(client, EPOLLIN);
					}
				} else {
					_removeClient(client); // reset connection and state in fail case
				}
			} catch (std::runtime_error &e) {
				_removeClient(client);  // reset connection and state in fail case
			}
        }
		if (j > EPOLL_ROUND_CLEANUP) {
			_handleTimeouts();
			j = 0;
		}
    }
}
