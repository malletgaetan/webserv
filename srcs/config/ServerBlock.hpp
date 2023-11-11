#pragma once

#include "config/LocationBlock.hpp"

class ServerBlock: public LocationBlock {
	private:
		std::vector<int> _ports;
		std::string _server_name;

		void	_parseServerName(const std::string &line);
	public:
		ServerBlock(std::ifstream &f, int default_port);
		~ServerBlock();
		void	printConfiguration(int indentation) const;
		bool	matchHost(const std::string &host) const;
		std::string getHost(void) const;
		const std::vector<int> &getPorts(void) const;
};
