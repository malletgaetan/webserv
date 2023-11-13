#pragma once

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
}

#include <stdexcept>

ssize_t send_with_throw(int sockFd, const std::string &text, const char *throwMsg);
