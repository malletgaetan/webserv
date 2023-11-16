#include "config/Config.hpp"
#include "server/Client.hpp"
#include "server/Server.hpp"
#include "server/utils.hpp"
#include "http.hpp"

# define CGI_INTERNAL_SERVER_ERROR 1
# define CGI_NOT_FOUND 2

// PUBLIC

Client::Client(int fd, const std::map<const std::string, const ServerBlock *> *servers_by_host): _servers_by_host(servers_by_host), _state(RECEIVING), _fd(fd), _config(NULL), _cgi_state(NONE)
{
	_last_activity = std::time(NULL);
}

Client::~Client()
{
	if (_cgi_state != NONE)
		_stopCGI();
	close(_fd);
}

State Client::getState(void)
{
	return _state;
}

int  Client::getFd(void)
{
	return _fd;
}

void Client::setLastActivity(std::time_t &now)
{
	_last_activity = now;
}

const std::time_t &Client::getLastActivity(void) const
{
	return _last_activity;
}

void Client::sendRequestTimeout(void)
{
	try {
		_http_status = HTTP_REQUEST_TIMEOUT_ERROR;
		_sendErrorResponse();
	} catch (std::runtime_error &e) {
		std::cerr << "Failed to send 408 Request Timeout to client." << std::endl;
	}
}

void Client::sendInternalServerError(void)
{
	_http_status = HTTP_INTERNAL_SERVER_ERROR;
	_sendErrorResponse();
	_state = RECEIVING;
}

bool Client::readHandler(void)
{
	ssize_t ret = recv(_fd, Server::_buf, SERVER_BUFFER_SIZE - 1, 0);
	Server::_buf[ret] = '\0';
	if (ret == -1)
		throw std::runtime_error("failed to read from client");
	if (ret == 0)
		return true;
	_request_buf.append(Server::_buf);
	_eor = _request_buf.find("\r\n\r\n"); // end of HTTP request, can be optimized to start searching from last index search
	if (_eor == std::string::npos)
		return false;
	_state = SENDING;
	return false;
}

void Client::writeHandler(void)
{
	(this->*_response_handler)();
}

void Client::_stopCGI(void)
{
	kill(_cgi_pid, SIGKILL);
	waitpid(_cgi_pid, NULL, 0); // is it usefull? kernel would collect
	_cgi_state = NONE;
}

static std::string extract_host(const std::string &request, size_t eor)
{
	size_t host_index = request.find("Host:");
	if (host_index == std::string::npos || host_index > eor)
		return std::string("");
	host_index += 5;
	while (isspace(request[host_index]))
		++host_index;
	size_t end_of_path = request.find("\r\n", host_index);
	size_t port_index = request.find(":", host_index);
	if (port_index == std::string::npos || port_index > eor)
		return request.substr(host_index, end_of_path - host_index);
	return request.substr(host_index, port_index - host_index);
}

static const std::string &get_mime(const std::string &filepath)
{
	size_t last_point = filepath.find_last_of('.');
	if (last_point == std::string::npos)
		last_point = filepath.size();
	return HTTP::mime_type(filepath.substr(last_point, filepath.size() - last_point));
}

static bool is_in_accept(const std::string &filepath, const std::string &request, size_t eor)
{
	const std::string &mime_type = get_mime(filepath);
	size_t accept_index = request.find("Accept:");
	if (accept_index == std::string::npos || accept_index > eor)
		return true;
	accept_index += 7;
	size_t accept_end = request.find("\r\n", accept_index);
	if (accept_end == std::string::npos || accept_end > eor)
		return true;
	size_t mime = request.find(mime_type, accept_index);
	if (mime != std::string::npos && mime <= eor)
		return true;
	mime = request.find("*/*", accept_index);
	if (mime == std::string::npos || mime > eor)
		return false;
	return true;
}

void Client::parseRequest(void)
{
	size_t first_space = _request_buf.find(' ');
	size_t second_space = _request_buf.find(' ', first_space + 1);

	try {
		if (first_space == std::string::npos || second_space == std::string::npos || first_space >= _eor)
			throw RequestParsingException(HTTP_BAD_REQUEST);

		switch (_request_buf[0]) { // bit hacky and bad
			case 'G':
				_method = HTTP::GET;
				break;
			case 'P':
				_method = HTTP::POST;
				break;
			case 'H':
				_method = HTTP::HEAD;
				break;
			case 'D':
				_method = HTTP::DELETE;
				break;
			default:
				throw RequestParsingException(HTTP_BAD_REQUEST);
		}

		const std::string path = _request_buf.substr(first_space + 1, second_space - first_space - 1);
		const std::string host = extract_host(_request_buf, _eor);
		if (host.size() == 0)
			throw RequestParsingException(HTTP_BAD_REQUEST);

		// match config
		_matchConfig(host, path);
		_config->printConfiguration(0);
		if (_config->isUnauthorizedMethod(_method))
			throw RequestParsingException(HTTP_METHOD_NOT_ALLOWED);		
		_static_filepath = _config->getFilepath(path);
	} catch (RequestParsingException &e) {
		_http_status = e.getHttpStatus();
		_response_handler = &Client::_sendErrorResponse;
		_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
		return ;
	}

	if (_config->isCGI(_static_filepath)) {
		_response_handler = &Client::_sendCGIResponse;
		_prepareCGI();
		return ;
	} else if (_config->isRedirect()) {
		_response_handler = &Client::_sendRedirectResponse;
	} else {
		if (!is_in_accept(_static_filepath, _request_buf, _eor)) {
			_http_status = HTTP_NOT_ACCEPTABLE;
			_response_handler = &Client::_sendErrorResponse;
		} else {
			_response_handler = &Client::_sendStaticResponse;
		}
	}

	// TODO check if auto_index actived to render folder page
	// TODO check that answer file is in accepted MIME types Accept header
	_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
}

// PRIVATE
int Client::_prepareCGI(void)
{
	int inpipe[2];

	if (pipe(_cgi_pipe) == -1)
		throw std::runtime_error("failed to create pipe");
	if (pipe(inpipe) == -1) {
		close(_cgi_pipe[0]);
		close(_cgi_pipe[1]);
		throw std::runtime_error("failed to create pipe");
	}
	const std::string request = _request_buf.substr(0, _eor + 4);
	_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
	if (write(inpipe[1], request.c_str(), request.size()) < 0) {
		close(inpipe[0]);
		close(inpipe[1]);
		close(_cgi_pipe[0]);
		close(_cgi_pipe[1]);
		throw std::runtime_error("failed to write to pipe");
	}
	_cgi_pid = fork();
	if (_cgi_pid == 0) {
		// TODO: redirect stderr to /dev/null
		if (close(inpipe[1]) < 0)
			exit(CGI_INTERNAL_SERVER_ERROR);
		if (close(_cgi_pipe[0]) < 0)
			exit(CGI_INTERNAL_SERVER_ERROR);

		if (dup2(inpipe[0], STDIN_FILENO) == -1)
			exit(CGI_INTERNAL_SERVER_ERROR);
		if (close(inpipe[0]) < 0)
			exit(CGI_INTERNAL_SERVER_ERROR);
		if (dup2(_cgi_pipe[1], STDOUT_FILENO) == -1)
			exit(CGI_INTERNAL_SERVER_ERROR);
		if (close(_cgi_pipe[1]) < 0)
			exit(CGI_INTERNAL_SERVER_ERROR);
		const char *arr[3];
		arr[0] = _config->getCGIExecutable().c_str();
		arr[1] = _static_filepath.c_str();
		arr[2] = NULL; 
		execve(arr[0],(char* const*)arr, NULL); // nice cast bro
		exit(CGI_NOT_FOUND);
	}
	close(inpipe[0]);
	close(inpipe[1]);
	close(_cgi_pipe[1]);
	if (_cgi_pid == -1)
		throw std::runtime_error("failed to fork cgi");
	_cgi_state = WAIT;
	_cgi_start = _last_activity;
	return _cgi_pipe[0];
}

void Client::_matchConfig(const std::string &host, const std::string &path)
{
	const std::map<const std::string, const ServerBlock *>::const_iterator it = _servers_by_host->find(host);
	const ServerBlock * server = it == _servers_by_host->end() ? _servers_by_host->begin()->second : it->second; // default server to the first one with same port

	_config = server->matchLocation(path, 0).second;
}

void Client::_prepareHeaders(std::stringstream &stream, size_t content_length, const std::string &extension)
{
	stream << "HTTP/1.1 " << _http_status << " " << HTTP::status_definition(_http_status) << "\r\n";
	if (_http_status != HTTP_PERMANENT_REDIRECT)
		stream << std::string("Content-Type: ") << HTTP::mime_type(extension) << "\r\n";
	else
		stream << std::string("Location: ") << _config->getRedirection() << "\r\n";
	if (_http_status == HTTP_INTERNAL_SERVER_ERROR) // in case of internal server error, reset connection and client internal state
		stream << std::string("Connection: close\r\n");
	stream << std::string("Content-Length: ") << content_length << std::string("\r\n\r\n");
}

void Client::_sendErrorResponse(void)
{
	const std::string &error = _config == NULL ? HTTP::default_error(_http_status) : _config->getErrorPage(_http_status);
	std::stringstream header_stream;

	_prepareHeaders(header_stream, error.size(), std::string(".html"));
	header_stream << error;
	send_with_throw(_fd, header_stream.str(), "failed to send error headers to client");
	_state = RECEIVING;
}

void Client::_sendStaticResponse(void)
{
	std::stringstream file_stream;
	std::stringstream header_stream;
	int	fd;

	fd = open(_static_filepath.c_str(), O_RDONLY);
	if (fd == -1) {
		_http_status = HTTP_NOT_FOUND;
		return _sendErrorResponse();
	}
	_http_status = HTTP_OK;

	ssize_t ret;

	// NOTES
	// 1 - reading everything could be seen as a bad idea, but its the only way to assure that we're sending the number of bytes that we told in header
	// 2 - with further reflection, we can't assure anything cause of network, it could fail at any point in time in request, so maybe just go back to sending
	// header and body in separate timing, without buffering files
	while ((ret = read(fd, Server::_buf, SERVER_BUFFER_SIZE - 1))) { // TODO cache file if performance low, or better manage file descriptors
		if (ret < 0) {
			close(fd);
			throw std::runtime_error("failed to read data from static file");
		}
		if (ret == 0)
			break ;
		file_stream.write(Server::_buf, ret);
	}
	close(fd);

	const std::string &mime_type = get_mime(_static_filepath);
	_prepareHeaders(header_stream, file_stream.str().size(), mime_type);
	header_stream << file_stream.str();

	send_with_throw(_fd, header_stream.str(), "failed to send file data to client");
	_state = RECEIVING;
}

void Client::_sendRedirectResponse(void)
{
	_http_status = HTTP_PERMANENT_REDIRECT;
	std::stringstream header_stream;
	_prepareHeaders(header_stream, 0, std::string());
	send_with_throw(_fd, header_stream.str(), "failed to send redirect to client");
	_state = RECEIVING;
}

void Client::_sendCGIResponse(void)
{
	if (_cgi_state == NONE) // CGI got _stopCGI() -> client is getting destroyed
		return ;

	if (std::difftime(_last_activity, _cgi_start) > GATEWAY_TIMEOUT) {
		_stopCGI();
		_cgi_stream.clear();
		_http_status = HTTP_GATEWAY_TIMEOUT;
		_response_handler = &Client::_sendErrorResponse;
		return ;
	}

	int wstatus;
	ssize_t ret;

	if (waitpid(_cgi_pid, &wstatus, WNOHANG) == 0) // not the most I/O design compliant but simple solution still better than total wait
		return ;

	if (!WIFEXITED(wstatus)) {
		_cgi_stream.clear();
		_http_status = HTTP_INTERNAL_SERVER_ERROR;
		_response_handler = &Client::_sendErrorResponse;
		close(_cgi_pipe[0]);
		return ;
	}
	
	_cgi_state = NONE;
	if (WEXITSTATUS(wstatus) != 0) {
		_cgi_stream.clear();
		_http_status = WEXITSTATUS(wstatus) == CGI_NOT_FOUND ? HTTP_NOT_FOUND : HTTP_INTERNAL_SERVER_ERROR;
		_response_handler = &Client::_sendErrorResponse;
		close(_cgi_pipe[0]);
		return ;
	}

	// read all from pipe to buffer
	while ((ret = read(_cgi_pipe[0], Server::_buf, SERVER_BUFFER_SIZE - 1))) { // TODO: repetiton of total read into stream, create function
		if (ret < 0) {
			close(_cgi_pipe[0]);
			throw std::runtime_error("failed to read output data from CGI");
		}
		if (ret == 0)
			break ;
		_cgi_stream.write(Server::_buf, ret);
	}
	close(_cgi_pipe[0]);
	send_with_throw(_fd, _cgi_stream.str(), "failed to send CGI to client");
	_state = RECEIVING;
	_cgi_stream.clear();
}
