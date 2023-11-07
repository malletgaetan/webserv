#pragma once

#include <unistd.h>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "config/ParserUtils.hpp"
#include "config/ServerBlock.hpp"
#include "RuntimeError.hpp"

int get_num_cpu_cores(void);

class Config {
	private:
		static std::vector<ServerBlock> _server_blocks;
		static int _nb_workers;
		
	public:
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
				} else if (line.compare(index, 7, std::string("workers")) == 0) {
					if (_nb_workers != 0)
						throw RuntimeError("cannot more than once workers attribute at line %zu column %zu", Config::line, index);
					index = skip_whitespaces(line, index + 7);
					_nb_workers = atoi(line.c_str() + index);
					index = expect_word_in_range(line, index, '0', '9');
					expect_end_of_content(line, index);
					if (_nb_workers < 1)
						throw RuntimeError("number of workers should be a postive integer at line %zu column %zu", Config::line, index);
				} else {
					throw RuntimeError("unrecognized attribute at line %zu column %zu", Config::line, index);
				}
				++Config::line;
			}
			if (_nb_workers == 0) {
				try {
					_nb_workers = get_num_cpu_cores(); // default to one worker per core
				} catch(std::exception &e) {
					throw RuntimeError("failed to get cpu info to create default number of workers");
				}
			}
		}
		static void printConfiguration(void)
		{
			for (std::vector<ServerBlock>::iterator it = _server_blocks.begin(); it != _server_blocks.end(); ++it) {
				it->printConfiguration(0);
			}
		}
		// const ServerBlock &matchServer(std::string &path)
		// {
		// 	// find closest ServerBlock configuration to path

		// }
};