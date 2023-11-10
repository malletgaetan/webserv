#pragma once

// TODO clean includes

extern "C" {
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
}

#include <iostream>
#include <stdexcept>
#include <map>
#include <ctime>

#define MAX_EVENTS 2048
#define LISTEN_BACKLOG 10
#define EPOLL_ROUND_CLEANUP 500 // clients cleanup every X calls to epoll_wait | cleanup every EPOLL_MAX_TIMEOUT * 500 max
#define CLIENT_TIMEOUT 120 // seconds
#define EPOLL_MAX_TIMEOUT 1000 // ms

#if SERVER_BUFFER_SIZE > SSIZE_MAX
	#error "SERVER_BUFFER_SIZE is too big, should not exceed SSIZE_MAX."
#endif

#if TCP_CLEAN_TICK > SIZE_MAX - EPOLL_BLOCK_MS - 1000
	#error "CLIENT_TIMEOUT_MS is too big."
#endif

#if CLIENT_TIMEOUT * 1000 < EPOLL_MAX_TIMEOUT
	#error "Unreachable CLIENT_TIMEOUT, should be more than EPOLL_MAX_TIMEOUT."
#endif

class Server
{
	private:
		std::map<int, Client *> _clients;
		std::vector<int> _server_fds;
		struct sockaddr_in _ssin, _csin;
		struct epoll_event _ev, *_events;
		socklen_t _socklen;
		int	_epfd;
		bool _running;
		void _handleTimeouts(void);
		void _replaceClientEvents(int clientfd, uint32_t op);
		void _addClient(void);
		void _removeClient(Client *client);
		void _eventLoop(void);
	public:
		Server();
		~Server();
		void serve(void);
		void stop(void);
};

