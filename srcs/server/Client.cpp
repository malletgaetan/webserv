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

// return (CGI fd)
int Client::parseRequest(void)
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
		if (_config->isUnauthorizedMethod(_method))
			throw RequestParsingException(HTTP_METHOD_NOT_ALLOWED);
	} catch (RequestParsingException &e) {
		_http_status = e.getHttpStatus();
		_response_handler = &Client::_sendErrorResponse;
		_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
		return (0);
	}

	if (_config->isCGI()) {
		_response_handler = &Client::_sendCGIResponse;
		return _prepareCGI();
	} else if (_config->isRedirect()) {
		_response_handler = &Client::_sendRedirectResponse;
	} else {
		_static_filepath = _config->getFilepath(_request_buf.substr(first_space + 1, second_space - first_space - 1));
		_response_handler = &Client::_sendStaticResponse;
	}

	// TODO check if auto_index actived to render folder page
	// TODO check that answer file is in accepted MIME types Accept header
	_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
	return (0);
}

// PRIVATE
//
int Client::_prepareCGI(void)
{
	if (pipe(_cgi_pipe) == -1)
		throw std::runtime_error("failed to create pipe");
	const std::string request = _request.substr(0, _eor + 4);
	if (write(_cgi_pipe[1], request.c_str(), request.size()) < 0)
		throw std::runtime_error("failed to write to pipe");
	_cgi_pid = fork();
	if (_cgi_pid == 0) {
		if (dup2(_cgi_pipe[0], STDIN_FILENO) != 0)
			exit(0);
		if (dup2(_cgi_pipe[1], STDOUT_FILENO) != 0)
			exit(0);
		execve();
			// TODO: here
	}
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

	size_t last_p = _static_filepath.find_last_of(".");
	const std::string mime_type = _static_filepath.substr(last_p, _static_filepath.size() - last_p);


	ssize_t ret;

	while ((ret = read(fd, Server::_buf, SERVER_BUFFER_SIZE))) {
		if (ret < 0) {
			close(fd);
			throw std::runtime_error("failed to read data from static file");
		}
		if (ret == 0)
			break ;
		file_stream.write(Server::_buf, ret);
	}
	close(fd);

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
	if (_cgi_stream.rdbuf()->in_avail() == 0)
		return ;
	send_with_throw(_fd, _cgi_stream.str(), "failed to send CGI to client");
	if (_cgi_pipe[0] == -1)
		_state = RECEIVING;
	_cgi_stream.clear();
}
