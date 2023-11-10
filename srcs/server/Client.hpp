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
	    typedef bool (Client::*ResponseHandler)();
		static char *buf;
		static ssize_t buf_size;
		static const Config *config;
		static void initializeBuffer(size_t size)
		{
			if (buf != NULL)
				return ;
			buf = new char[size];
			if (!buf) 
				throw std::runtime_error("Failed to allocate Client Buffer.");
			buf_size = size;
		}
		static void initializeConfig(const Config *c)
		{
			config = c;
		}
		static void destroyBuffer()
		{
			if (buf == NULL)
				return ;
			delete buf;
		}
		Client(int fd);
		~Client(void);
		int getState(void);
		int  getFd(void);
		int getPipe(void);
		void setLastActivity(std::time_t &now);
		const std::time_t &getLastActivity(void) const;
		void requestTimeout(void);
		void internalServerError(void);
		bool consume(void);
		bool answer(void);
		void parseRequest(void);
		int  startCGI(void);
		void stopCGI(void);
		bool isCGI(void);
	private:
		size_t eor; // end of request
		std::string http_path;
		State state;
		Method method;
		int fd, fddata, http_status, pipe[2];
		std::string request_buf;
		const std::string *errortosend;
		std::time_t last_activity_time;
		ResponseHandler responseHandler;

		void setPath(std::string path);
		bool setMethod(size_t end);
		void sendErrorHeaders(void);
		void sendErrorResponse(void);
		void sendHeaders(void);
		bool sendStaticResponse(void);
		bool sendCGIResponse(void);
		bool sendRedirectResponse(void);
};
