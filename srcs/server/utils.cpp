#include "server/utils.hpp"

ssize_t send_with_throw(int sockFd, const std::string &text, const char *throwMsg)
{
	ssize_t ret;
	if ((ret = send(sockFd, text.c_str(), text.size(), 0)) < 0) // TODO ret could have send less than len bytes, need to check this case and handle it correctly
		throw std::runtime_error(throwMsg);
	return ret;
}
