#include "server/Client.cpp"

// PUBLIC

Client::Client()
{
	if (Client::_buf == NULL) {
		Client::_buf = new char[CLIENT_BUFFER_SIZE];
		if (!_buf) 
			throw std::runtime_error("failed to allocate client buffer");
	}
}

Client::~Client()
{
	if (buf == NULL)
		return ;
	delete buf;
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
	return ;
}

void Client::sendInternalServerError(void)
{
	return ;
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
	return ;
}

// PRIVATE
static char *Client::_buf = NULL;

void Client::_sendErrorHeaders(void);
void Client::_sendErrorResponse(void);
void Client::_sendHeaders(void);
bool Client::_sendStaticResponse(void);
bool Client::_sendRedirectResponse(void);
