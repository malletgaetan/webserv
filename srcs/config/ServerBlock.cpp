#include <iostream>
#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include "config/utils.hpp"

ServerBlock::ServerBlock(std::ifstream &f, int default_port): LocationBlock()
{
	for (std::string line; std::getline(f, line);) {
		++Config::line;
		_index = skip_whitespaces(line, 0);
		if (_index == line.size() || line[_index] == '#' || line[_index] == ';')
			continue ;
		if (line[_index] == '}') {
			if (_ports.size() == 0)
				_ports.push_back(default_port);
			return ;
		}
		if (line.compare(_index, 6, std::string("listen")) == 0) {
			_index += 6;
			while (line[_index] != ';') {
				int new_port = _parseInt(line);
				for (size_t i = 0; i < _ports.size(); i++) {
					if (_ports[i] == new_port)
						throw ConfigParsingException("cannot listen twice on the same port at line %zu", Config::line);
				}
				_ports.push_back(new_port);

			}
			if (_ports.back() < 0 || _ports.back() > 65535)
				throw ConfigParsingException("invalid range for listen port at line %zu column %zu", Config::line, _index);
			expect_end_of_content(line, _index);
		} else if (line.compare(_index, 11, std::string("server_name")) == 0) {
			_index += 11;
			_parseServerName(line);
		} else {
			_parseAttribute(line, f);
		}
	}
	throw ConfigParsingException("expected '}' but got EOF", Config::line, _index);
}

ServerBlock::~ServerBlock()
{
}

void	ServerBlock::_parseServerName(const std::string &line)
{
	_index = skip_whitespaces(line, _index);
	bool point = false;
	size_t start_index = _index;
	while (true) {
		if (_index == line.size())
			throw ConfigParsingException("unexpected 'newline' at line %zu column %zu", Config::line, _index);
		if (line[_index] == ';')
			break ;
		if (line[_index] <= 'z' && line[_index] >= 'a') {
			point = true;
		} else if (line[_index] == '.') {
			if (point == false) {
				throw ConfigParsingException("malformed location path at line %zu column %zu", Config::line, _index);
			}
			point = false;
		} else {
			throw ConfigParsingException("uncompatible character in server_name directive at line %zu column %zu", Config::line, _index);
		}
		++_index;
	}
	_server_name = line.substr(start_index, _index - start_index);
	if (_server_name.size() == 0)
		throw ConfigParsingException("missing server_name value at line %zu", Config::line);
}


std::string ServerBlock::getHost(void) const
{
	return _server_name;
}

void ServerBlock::printConfiguration(int indentation) const
{
	std::cout << generate_tabs(indentation) << "Server {" << std::endl;
	std::cout << generate_tabs(indentation + 1) << "listen";
	for (std::vector<int>::const_iterator it = _ports.begin(); it != _ports.end(); ++it) {
		std::cout << " " << *it;
	}
	std::cout << std::endl;
	if (_server_name.size() != 0)
		std::cout << generate_tabs(indentation + 1) << "server_name " << _server_name << std::endl;
	_printState(indentation + 1);
	std::cout << generate_tabs(indentation) << "}" << std::endl;
}

const std::vector<int> &ServerBlock::getPorts(void) const
{
	return _ports;
}

