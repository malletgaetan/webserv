#pragma once

#include <unistd.h>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "config/utils.hpp"
#include "config/ServerBlock.hpp"
#include "RuntimeError.hpp"

int get_num_cpu_cores(void);

class Config {
	private:
		static std::vector<ServerBlock> _server_blocks;
		
	public:
		std::vector<int>	ports;
		static size_t	line;
		static void parseFile(const char *filepath)
		{
			std::ifstream fs(filepath, std::ifstream::in);
			for (std::string line; std::getline(fs, line); ) {
				++Config::line;
				size_t	index = skip_whitespaces(line, 0);
				if (index == line.size() || line[index] == ';' || line[index] == '#') {
					continue ;
				}
				if (line.compare(index, 6, std::string("server")) == 0) {
					index = skip_whitespaces(line, index + 6);
					index = expect_char(line, index, '{');
					index = skip_whitespaces(line, index);
					_server_blocks.push_back(ServerBlock(fs));
				} else {
					throw RuntimeError("unrecognized attribute at line %zu column %zu", Config::line, index);
				}
			}
		}
		static void printConfiguration(void)
		{
			for (std::vector<ServerBlock>::iterator it = _server_blocks.begin(); it != _server_blocks.end(); ++it) {
				it->printConfiguration(0);
			}
		}
		static const LocationBlock *matchConfig(const std::string &host, const std::string &path)
		{
			std::vector<ServerBlock>::iterator it = _server_blocks.begin(); // default to first server declaration
			ServerBlock &s = *it;
			for (; it != _server_blocks.end(); ++it) {
				if (it->matchHost(host)) {
					s = *it;
					break ;
				}
			}
			return s.matchLocation(path, 0);
		}
};
