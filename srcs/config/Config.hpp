#pragma once

#include <unistd.h>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "config/utils.hpp"
#include "config/ServerBlock.hpp"
#include "config/ConfigParsingException.hpp"

int get_num_cpu_cores(void);

class Config {
	private:
		static std::vector<ServerBlock> _servers;
		static std::map<int, std::map<const std::string, const ServerBlock *> > _ordered_servers;

	public:
		static size_t	line;
		static std::vector<int> ports;
		static void parseFile(const char *filepath)
		{
			std::ifstream fs(filepath, std::ifstream::in);
			int	default_port = geteuid() ? 8000 : 80;
			for (std::string line; std::getline(fs, line); ) {
				++Config::line;
				size_t	index = skip_whitespaces(line, 0);
				if (index == line.size() || line[index] == ';' || line[index] == '#')
					continue ;
				if (line.compare(index, 6, std::string("server")) == 0) {
					index = skip_whitespaces(line, index + 6);
					index = expect_char(line, index, '{');
					index = skip_whitespaces(line, index);
					_servers.push_back(ServerBlock(fs, default_port));
					const std::vector<int> &ports = _servers.back().getPorts();
					for (std::vector<int>::const_iterator it = ports.begin(); it != ports.end(); ++it) {
						_ordered_servers[*it][_servers.back().getHost()] = &_servers.back();
					}
				} else {
					throw ConfigParsingException("unrecognized attribute at line %zu column %zu", Config::line, index);
				}
			}
			// TODO: this could be implemented an other way
			for (std::map<int, std::map<const std::string, const ServerBlock *> >::const_iterator it = _ordered_servers.begin(); it != _ordered_servers.end(); ++it)
				ports.push_back(it->first);
			if (_servers.size() == 0)
				throw ConfigParsingException("empty configuration or invalid configuration, locations should be in a server block");
		}
		static void printConfiguration(void)
		{
			for (std::vector<ServerBlock>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
				it->printConfiguration(0);
		}
		static const std::map<const std::string, const ServerBlock *> *getServers(int port)
		{
			return &_ordered_servers[port];
		}
};
