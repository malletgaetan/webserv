#pragma once

#include <unistd.h>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "config/ParserUtils.hpp"
#include "config/ServerBlock.hpp"
#include "RuntimeError.hpp"

int get_num_cpu_cores(void)
{
    std::ifstream cpuinfo("/proc/cpuinfo");
    int numCores = 0;
    std::string line;

    while (std::getline(cpuinfo, line)) {
        if (line.substr(0, 9) == "processor") {
            numCores++;
        }
    }

    return numCores;
}

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
				// match "server" | "worker"
				size_t	index = skip_whitespaces(line, 0);
				if (line[index] == ';') {
					++Config::line;
					continue ;
				}
				if (line.compare(index, 6, std::string("server")) == 0) {
					index = expect_word_in_range(line, index + 6, '{', '{');
					index = expect_char(line, index + 1, ';');
					index = skip_whitespaces(line, index);
					if (index != line.size())
						throw RuntimeError("expected end of line at line %zu column %zu", Config::line, index);
					_server_blocks.push_back(ServerBlock(fs));
				} else if (line.compare(index, 7, std::string("workers")) == 0) {
					if (_nb_workers != 0)
						throw RuntimeError("cannot more than once workers attribute at line %zu column %zu", Config::line, index);
					index = skip_whitespaces(line, index + 7);
					_nb_workers = atoi(line.c_str() + index);
					index = expect_word_in_range(line, index, '0', '9');
					index = expect_char(line, index, ';');
					if (index != line.size())
						throw RuntimeError("expected end of line at line %zu column %zu", Config::line, index);
					if (_nb_workers < 1)
						throw RuntimeError("number of workers should be a postive integer at line %zu column %zu", Config::line, index);
				} else {
					throw RuntimeError("unregonized attribute at line %zu column %zu", Config::line, index);
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
		const Server &matchServer(std::string &path)
		{
			// find closest ServerBlock configuration to path

		}
};