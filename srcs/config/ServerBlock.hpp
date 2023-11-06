#pragma once

#include "config/Block.hpp"

class ServerBlock: public Block {
	private:
		int	_listen;
		std::string _host;
		std::string _server_name;
	public:
		ServerBlock();
		~ServerBlock();
};
