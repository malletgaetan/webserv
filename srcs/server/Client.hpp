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

#include "config/LocationBlock.hpp"
#include "server/Client.hpp"

enum State {
	RECEIVING,
	PARSING,
	SENDING
};

class RequestParsingException : public std::exception {
	public:
		RequestParsingException(int http_status)
		{
			_http_status = http_status;
		}
		virtual const char* what() const throw()
		{
			return "";
		}
		int getHttpStatus(void)
		{
			return _http_status;
		}
	private:
		int _http_status;
};

class Client
{
	public:
		Client(int fd, const std::map<const std::string, const ServerBlock *> *servers_by_host);
		~Client(void);
		State getState(void);
		int  getFd(void);
		void setLastActivity(std::time_t &now);
		const std::time_t &getLastActivity(void) const;
		void sendRequestTimeout(void);
		void sendInternalServerError(void);
		bool readHandler(void); // return whether client should be deleted
		void writeHandler(void);
		void parseRequest(void);
	private:
	    typedef void (Client::*ResponseHandler)(void);
		static char *_buf;
		const std::map<const std::string, const ServerBlock *> *_servers_by_host;
		size_t _eor; // end of last request
		State _state;
		HTTP::Method _method;
		int _fd, _static_fd, _http_status;
		std::string _static_filepath, _request_buf;
		std::time_t _last_activity;
		ResponseHandler _response_handler;
		const LocationBlock *_config;
		std::stringstream _response_stream;

		void _matchConfig(const std::string &host, const std::string &path);
		void _sendHeaders(size_t content_length, const std::string &extension);
		void _sendErrorResponse(void);
		void _sendStaticResponse(void);
		/* bool _sendCGIResponse(void); */
		void _sendRedirectResponse(void);
};
