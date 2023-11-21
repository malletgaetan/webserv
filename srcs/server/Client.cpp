#include "config/Config.hpp"
#include "server/Client.hpp"
#include "server/Server.hpp"
#include "server/utils.hpp"
#include "http.hpp"

# define CGI_INTERNAL_SERVER_ERROR 1
# define CGI_NOT_FOUND 2

// PUBLIC

Client::Client(int fd, const std::map<const std::string, const ServerBlock *> *servers_by_host): _servers_by_host(servers_by_host), _state(REQUEST_START_WAIT), _fd(fd), _read_handler(&Client::_requestHandler), _config(NULL), _cgi_state(NONE)
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
	_state = REQUEST_START_WAIT;
}

void Client::readHandler(void)
{
	(this->*_read_handler)();
}

void Client::_bodyHandler(void)
{
	ssize_t ret = recv(_fd, Server::_buf, SERVER_BUFFER_SIZE - 1, 0);
	if (ret == -1)
		throw std::runtime_error("failed to read body from client");
	Server::_buf[ret] = '\0';
	_request_buf.append(Server::_buf);
	if ((size_t)_body_size <= _request_buf.size()) {
		_createFile();
		_response_handler = &Client::_sendOk;
		_read_handler = &Client::_requestHandler;
		_state = ANSWER;
	}
}

void Client::_requestHandler(void)
{
	ssize_t ret = recv(_fd, Server::_buf, SERVER_BUFFER_SIZE - 1, 0);
	if (ret == -1)
		throw std::runtime_error("failed to read request from client");

	Server::_buf[ret] = '\0';
	_request_buf.append(Server::_buf);

	// search for start of request
	if (_state == REQUEST_START_WAIT) {
		while (true) {
			if (_request_buf.size() == 0)
				return ;
			size_t request_identifier = _request_buf.find("HTTP/1.1\r\n"); // start of request
			if (request_identifier == std::string::npos) {
				_request_buf.clear();
				return ;
			}
			ssize_t sor = request_identifier - 1;  // start of request
			int space_nb = 0;
			while (true) {
				if (sor < 0) {
					if (space_nb == 2)
						++sor;
					break ;
				}
				if (isspace(_request_buf[sor])) {
					if (space_nb == 2) {
						++sor;
						break;
					}
					++space_nb;
					while (sor >= 0 && isspace(_request_buf[sor]))
						--sor;
				} else {
					--sor;
				}
			}
			if (space_nb != 2 || sor < 0) {
				_request_buf.erase(0, request_identifier + 12);
				continue ;
			}
			_request_buf.erase(0, sor);
			break ;
		}
		_state = REQUEST_END_WAIT;
	}
	// search for end of request
	_eor = _request_buf.find("\r\n\r\n"); // end of HTTP request, can be optimized to start searching from last index search
	if (_eor == std::string::npos)
		return ;
	try {
		_parseRequest();
	} catch (RequestParsingException &e) {
		_http_status = e.getHttpStatus();
		_response_handler = &Client::_sendErrorResponse;
	}
	_request_buf.erase(0, _eor + 4);
}

void Client::writeHandler(void)
{
	(this->*_response_handler)();
}

static std::string extract_header_value(const std::string &request, const std::string &header, size_t eor)
{
	size_t header_index = request.find(header);
	if (header_index == std::string::npos || header_index > eor)
		return std::string("");
	header_index += header.size();
	while (isspace(request[header_index]))
		++header_index;
	size_t end_of_header = request.find("\r\n", header_index);
	if (end_of_header == std::string::npos || end_of_header > eor)
		return std::string("");
	return request.substr(header_index, end_of_header - header_index);
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

void Client::_setMethod(void)
{
	// this can't crash, since there is at least "\r\n\r\n" in _request_buf
	switch (_request_buf[0]) {
		case 'G':
			_method = HTTP::GET;
			break;
		case 'P':
			_method = HTTP::POST;
			break;
		case 'D':
			_method = HTTP::DELETE;
			break;
		default:
			throw RequestParsingException(HTTP_BAD_REQUEST);
	}
}

void Client::_parseRequest(void)
{
	size_t first_space = _request_buf.find(' ');
	size_t second_space = _request_buf.find(' ', first_space + 1);

	_state = ANSWER;
	if (first_space == std::string::npos || second_space == std::string::npos || first_space >= _eor)
		throw RequestParsingException(HTTP_BAD_REQUEST);

	_setMethod();

	const std::string path = _request_buf.substr(first_space + 1, second_space - first_space - 1);
	std::string host = extract_header_value(_request_buf, std::string("Host:"), _eor);
	size_t port_index = host.find(":");
	if (port_index != std::string::npos)
		host = host.substr(0, port_index);
	if (host.size() == 0)
		throw RequestParsingException(HTTP_BAD_REQUEST);

	// match config
	_matchConfig(host, path);
	if (_config->isUnauthorizedMethod(_method))
		throw RequestParsingException(HTTP_METHOD_NOT_ALLOWED);		
	_static_filepath = _config->getFilepath(path);

	// identify query
	if (_config->isCGI(_static_filepath)) {
		_response_handler = &Client::_sendCGIResponse;
		_prepareCGI();
		return ;
	} else if (_config->isRedirect()) {
		_response_handler = &Client::_sendRedirectResponse;
		return ;
	}

	if (!_config->isAutoIndex() && (_static_filepath[_static_filepath.size() - 1] == '/' || _static_filepath.size() == 0))
		throw RequestParsingException(HTTP_FORBIDDEN);
	if (_method == HTTP::DELETE) {
		if (_static_filepath[_static_filepath.size() - 1] == '/')
			throw RequestParsingException(HTTP_METHOD_NOT_ALLOWED);
		_deleteFile();
		_response_handler = &Client::_sendOk;
	} else if (_method == HTTP::GET) {
		if (!is_in_accept(_static_filepath, _request_buf, _eor))
			throw RequestParsingException(HTTP_NOT_ACCEPTABLE);
		_response_handler = &Client::_sendStaticResponse;
	} else {
		// INVESTIGATE: can't upload more that MAX_INT byte
		if (_static_filepath[_static_filepath.size() - 1] == '/')
			throw RequestParsingException(HTTP_METHOD_NOT_ALLOWED);
		int max_size = _config->getBodyLimit();
		if (max_size == 0)
			max_size = INT_MAX;
		std::string content_length = extract_header_value(_request_buf, std::string("Content-Length:"), _eor);
		if (content_length.size() == 0)
			throw RequestParsingException(HTTP_LENGTH_REQUIRED);
		_body_size = atoi(content_length.c_str());
		if (_body_size <= 0)
			throw RequestParsingException(HTTP_LENGTH_REQUIRED); // INVESTIGATE: hmh, how to handle this case? BAD_REQUEST?
		// do we already have the data?
		if (_body_size > _config->getBodyLimit())
			throw RequestParsingException(HTTP_PAYLOAD_TOO_LARGE);
		if (_request_buf.size() - _eor >= (size_t)_body_size) {
			_createFile();
			_response_handler = &Client::_sendOk;
			return ;
		}
		_read_handler = &Client::_bodyHandler;
		_state = REQUEST_START_WAIT;
	}
}

void Client::_deleteFile(void)
{
	if (remove(_static_filepath.c_str()) != 0)
		throw RequestParsingException(HTTP_NOT_FOUND);
}

void Client::_createFile(void)
{
	int fd = open(_static_filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
		throw RequestParsingException(HTTP_INTERNAL_SERVER_ERROR);
	if (write(fd, _request_buf.substr(_eor + 4, _body_size).c_str(), _body_size) < 0) // TODO check that the whole file was written
		throw RequestParsingException(HTTP_INTERNAL_SERVER_ERROR);
}

// PRIVATE
// TODO: CGI need to be executed with correct relative path
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
	_state = REQUEST_START_WAIT;
}

void Client::_sendStaticResponse(void)
{
	std::stringstream body_stream;
	std::stringstream header_stream;
	struct stat file_stat;
	struct dirent *entry;
	int	fd;

	fd = open(_static_filepath.c_str(), O_RDONLY);
	if (fd == -1) {
		_http_status = HTTP_NOT_FOUND;
		return _sendErrorResponse();
	}
    if (fstat(fd, &file_stat) == -1) {
		close(fd);
		_http_status = HTTP_INTERNAL_SERVER_ERROR;
		return _sendErrorResponse();
    }

    // Check if the file descriptor corresponds to a directory
	_http_status = HTTP_OK;
	if (S_ISDIR(file_stat.st_mode) && !_config->isAutoIndex()) {
		close(fd);
		_http_status = HTTP_NOT_FOUND;
		return _sendErrorResponse();
	}
    if (S_ISDIR(file_stat.st_mode)) {
		// TODO: INVESTIGATE: what happen if some inner location has a redirect?? true path wouldn't go to file !
		// send directory
		DIR *dir = fdopendir(fd);

		if (dir == NULL) {
			close(fd);
			throw std::runtime_error("failed to open directory");
		}

		// Start constructing HTML
		body_stream << "<html>\n<head>\n<title>Index of Folder</title>\n</head>\n<body>\n";
		body_stream << "<h1>Index of Folder</h1>\n";
		body_stream << "<ul>\n";

		// Read directory entries and generate HTML representation
		while ((entry = readdir(dir)) != NULL) {
			if (fstatat(fd, entry->d_name, &file_stat, 0) == -1)
				continue;
			body_stream << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>\n";
		}

		body_stream << "</ul>\n</body>\n</html>\n";
    } else {
		// send file
		ssize_t ret;

		// keep it simple
		while ((ret = read(fd, Server::_buf, SERVER_BUFFER_SIZE - 1))) { // TODO cache file if performance low, or better manage file descriptors
			if (ret < 0) {
				close(fd);
				throw std::runtime_error("failed to read data from static file");
			}
			if (ret == 0)
				break ;
			body_stream.write(Server::_buf, ret);
		}
	}
	close(fd);

	size_t last_point = _static_filepath.find_last_of('.');
	if (last_point == std::string::npos)
		last_point = _static_filepath.size();
	_prepareHeaders(header_stream, body_stream.str().size(), _static_filepath.substr(last_point, _static_filepath.size() - last_point));
	header_stream << body_stream.str();
	send_with_throw(_fd, header_stream.str(), "failed to send file data to client");
	_state = REQUEST_START_WAIT;
}

void Client::_sendRedirectResponse(void)
{
	_http_status = HTTP_PERMANENT_REDIRECT;
	std::stringstream header_stream;
	_prepareHeaders(header_stream, 0, std::string());
	send_with_throw(_fd, header_stream.str(), "failed to send redirect to client");
	_state = REQUEST_START_WAIT;
}

void Client::_stopCGI(void)
{
	kill(_cgi_pid, SIGKILL);
	waitpid(_cgi_pid, NULL, 0); // is it usefull? kernel would collect
	_cgi_state = NONE;
}

// TODO: INVESTIGATE: this seems pretty slow, could this be just python slow ?
void Client::_sendCGIResponse(void)
{
	if (_cgi_state == NONE) // just in case
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
	_state = REQUEST_START_WAIT;
	_cgi_stream.clear();
}


void Client::_sendOk(void)
{
	_http_status = HTTP_OK;
	std::stringstream header_stream;
	_prepareHeaders(header_stream, 0, std::string());
	send_with_throw(_fd, header_stream.str(), "failed to send OK to client");
	_state = REQUEST_START_WAIT;
}
