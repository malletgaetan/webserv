#pragma once

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
}

#include <stdexcept>

ssize_t send_with_throw(int sockFd, const void *buf, size_t len, const char * throwMsg);
