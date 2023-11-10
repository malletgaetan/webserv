#pragma once

extern "C" {
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
}

#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <sstream>
#include <ctime>

enum State {
	RECEIVING,
	PARSING,
	SENDING
};

enum Method {
	GET,
	HEAD,
	POST,
	DELETE
};

class Client
{
	public:
		Client(int fd);
		~Client(void);
		int getState(void);
		int  getFd(void);
		void setLastActivity(std::time_t &now);
		const std::time_t &getLastActivity(void) const;
		void sendRequestTimeout(void);
		void sendInternalServerError(void);
		bool readHandler(void);
		bool writeHandler(void);
		void parseRequest(void);
	private:
	    typedef bool (Client::*ResponseHandler)();
		static char *_buf;
		size_t _eor; // end of last request
		State _state;
		Method _method;
		int _fd, _static_fd, _http_status;
		std::string _http_path, _request_buf;
		std::time_t _last_activity;
		ResponseHandler _response_handler;
		const LocationBlock *config;

		void _sendErrorHeaders(void);
		void _sendErrorResponse(void);
		void _sendHeaders(void);
		bool _sendStaticResponse(void);
		bool _sendCGIResponse(void);
		bool _sendRedirectResponse(void);
};
