#include "server/Client.cpp"

// PUBLIC

Client::Client()
{
	if (_buf == NULL) {
		_buf = new char[CLIENT_BUFFER_SIZE];
		if (!_buf) 
			throw std::runtime_error("failed to allocate client buffer");
	}
}

Client::~Client()
{
	if (_buf == NULL)
		return ;
	delete _buf;
	_buf = NULL;
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
		this->http_status = 408;
		this->sendErrorResponse();
	} catch (std::runtime_error &e) {
		std::cerr << "Failed to send 408 Request Timeout to client." << std::endl;
	}
}

void Client::sendInternalServerError(void)
{
	this->http_status = 500;
	this->sendErrorResponse();
	this->state = RECEIVING;
}

bool Client::readHandler(void)
{
	return true;
}

bool Client::writeHandler(void)
{
	return true;
}

void Client::parseRequest(void)
{
	_config = config;
	size_t first_space = _request_buf.find(' '); // TODO: should only parse on current request
	size_t second_space = _request_buf.find(' ', first_space + 1);

	// TODO read the host header, and matchHost with host header, path and port
	if (first_space == std::string::npos || second_space == std::string::npos || first_space >= this->eor) {
		_http_status = 400;
		_response_handler = &_sendErrorResponse;
		return ;
	}

	switch (_request_buf[0]) { // bit hacky
		case 'G':
			this->method = GET;
			break;
		case 'P':
			this->method = POST;
			break;
		case 'H':
			this->method = HEAD;
			break;
		case 'D':
			this->method = DELETE;
			break;
		default:
			_http_status = 400;
			_response_handler = &_sendErrorResponse;
			return ;
	}

	_http_path = config->getRoot() + _request_buf.substr(first_space + 1, second_space - first_space - 1);
	if (_http_path.back() == "/")
		_http_path = _http_path + config->getIndex();
	_request_buf.erase(0, _eor + 4); // _eor + 4 end characters of HTTP request
}

// PRIVATE
static char *Client::_buf = NULL;

void Client::_sendErrorHeaders(void)
{
	return ;
}

void Client::_sendHeaders(void)
{
	return ;
}

void Client::_sendErrorResponse(void)
{
	return ;
}


bool Client::_sendStaticResponse(void)
{
	return true;
}

bool Client::_sendRedirectResponse(void)
{
	return true;
}
