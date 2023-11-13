#include "config/Config.hpp"
#include "server/Client.hpp"
#include "server/Server.hpp"
#include "server/utils.hpp"
#include "http.hpp"

// PUBLIC

Client::Client(int fd, const std::map<const std::string, const ServerBlock *> *servers_by_host): _servers_by_host(servers_by_host), _state(RECEIVING), _fd(fd), _config(NULL)
{
	_last_activity = std::time(NULL);
}

Client::~Client()
{
	close(_fd);
	close(_static_fd);
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
	_state = PARSING;
	return false;
}

void Client::writeHandler(void)
{
	(this->*_response_handler)();
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

void Client::parseRequest(void)
{
	// TODO: if request isn't HTTP1.1??
	size_t first_space = _request_buf.find(' '); // TODO: should only parse on current request
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
		if (_config->isUnauthorizedMethod(_method))
			throw RequestParsingException(HTTP_METHOD_NOT_ALLOWED);

		_static_filepath = _config->getFilepath(_request_buf.substr(first_space + 1, second_space - first_space - 1));
		// TODO check if auto_index actived to render folder page
		// TODO add CGI support
		// TODO check that answer file is in accepted MIME types Accept header
		_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
		_response_handler = &Client::_sendStaticResponse;
	} catch (RequestParsingException &e) {
		_http_status = e.getHttpStatus();
		_response_handler = &Client::_sendErrorResponse;
		_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
	}
}

// PRIVATE
void Client::_matchConfig(const std::string &host, const std::string &path)
{
	const std::map<const std::string, const ServerBlock *>::const_iterator it = _servers_by_host->find(host);
	const ServerBlock * server = it == _servers_by_host->end() ? _servers_by_host->begin()->second : it->second; // default server to the first one with same port

	_config = server->matchLocation(path, 0).second;
}

void Client::_sendHeaders(size_t content_length, const std::string &extension)
{
	std::ostringstream headers;
	headers << "HTTP/1.1 " << _http_status << " " << HTTP::status_definition(_http_status) << "\r\n";
	headers << std::string("Content-Type: ") << HTTP::mime_type(extension) << "\r\n";
	headers << std::string("Content-Length: ") << content_length << std::string("\r\n\r\n");
	std::string h = headers.str();
	send_with_throw(_fd, h.c_str(), h.size(), "failed to send headers to client");
}

void Client::_sendErrorResponse(void)
{
	const std::string &error = _config == NULL ? HTTP::default_error(_http_status) : _config->getErrorPage(_http_status);
	if (_state != SENDING) {
		_sendHeaders(error.size(), std::string(".html")); // errors are all html pages, set LocationBlock::parseError() if needed
		_state = SENDING;
	}
	send_with_throw(_fd, error.c_str(), error.size(), "failed to send error headers to client"); // this could be buffered in multi send
	_state = RECEIVING;
}

static ssize_t get_file_size(int fd)
{
	off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1)
		throw std::runtime_error("failed to get size of static file");
	if (lseek(fd, 0, SEEK_SET) == -1)
		throw std::runtime_error("failed to get size of static file");
    return file_size;
}

void Client::_sendStaticResponse(void)
{
	if (_state == PARSING) {
		_static_fd = open(_static_filepath.c_str(), O_RDONLY);
		if (_static_fd == -1) {
			_http_status = HTTP_NOT_FOUND;
			return _sendErrorResponse();
		}
		_http_status = HTTP_OK;
		_state = SENDING;
		size_t last_p = _static_filepath.find_last_of(".");
		const std::string mime_type = _static_filepath.substr(last_p, _static_filepath.size() - last_p);
		_sendHeaders(get_file_size(_static_fd), mime_type);
	}
	ssize_t ret = read(_static_fd, Server::_buf, SERVER_BUFFER_SIZE);
	if (ret == -1)
		throw std::runtime_error("failed to read data from static file");
	if (ret == 0) {
		close(_static_fd); // TODO could cause bug in case of throw, static_fd wouldn't be closed
		_static_fd = -1;
		_state = RECEIVING;
	}
	Server::_buf[ret] = '\0';
	send_with_throw(_fd, Server::_buf, ret, "failed to send file data to client");
}

void Client::_sendRedirectResponse(void)
{
	return ;
}

