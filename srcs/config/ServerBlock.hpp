#pragma once

#include "config/LocationBlock.hpp"

class ServerBlock: public LocationBlock {
	private:
		int	_listen;
		std::string _server_name;
	public:
		ServerBlock(std::ifstream &f);
		~ServerBlock();
		void	parseServerName(const std::string &line);
		void	printConfiguration(int indentation) const;
		bool	matchHost(const std::string &host);
};
