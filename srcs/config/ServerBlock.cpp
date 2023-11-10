#include <iostream>
#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include "config/utils.hpp"

ServerBlock::ServerBlock(std::ifstream &f): LocationBlock()
{
	for (std::string line; std::getline(f, line);) {
		++Config::line;
		_index = skip_whitespaces(line, 0);
		if (_index == line.size() || line[_index] == '#' || line[_index] == ';')
			continue ;
		if (line[_index] == '}')
			return ;
		if (line.compare(_index, 6, std::string("listen")) == 0) {
			_index += 6;
			this->_listen = this->parseInt(line);
			expect_end_of_content(line, _index);
		} else if (line.compare(_index, 11, std::string("server_name")) == 0) {
			_index += 11;
			this->parseServerName(line);
		} else {
			this->parseAttribute(line, f);
		}
	}
	if (_listen < 0 || _listen > 65535)
		throw RuntimeError("invalid range for listen port at line %zu", Config::line);
	Config::ports.push_back(_listen);
	throw RuntimeError("expected '}' but got EOF", Config::line, _index);
}

ServerBlock::~ServerBlock()
{
}

void	ServerBlock::parseServerName(const std::string &line)
{
	_index = skip_whitespaces(line, _index);
	bool point = false;
	size_t start_index = _index;
	while (true) {
		if (_index == line.size())
			throw RuntimeError("unexpected 'newline' at line %zu column %zu", Config::line, _index);
		if (line[_index] == ';')
			break ;
		if (line[_index] <= 'z' && line[_index] >= 'a') {
			point = true;
		} else if (line[_index] == '.') {
			if (point == false) {
				throw RuntimeError("malformed location path at line %zu column %zu", Config::line, _index);
			}
			point = false;
		} else {
			throw RuntimeError("uncompatible character in server_name directive at line %zu column %zu", Config::line, _index);
		}
		++_index;
	}
	this->_server_name = line.substr(start_index, _index - start_index);
}


bool	ServerBlock::matchHost(const std::string &host) const
{
	return host == _server_name;
}

void ServerBlock::printConfiguration(int indentation) const
{
	std::cout << generate_tabs(indentation) << "Server {" << std::endl;
	std::cout << generate_tabs(indentation + 1) << "listen " << _listen << std::endl;
	if (_server_name.size() != 0)
		std::cout << generate_tabs(indentation + 1) << "server_name " << _server_name << std::endl;
	this->printState(indentation + 1);
	std::cout << generate_tabs(indentation) << "}" << std::endl;
}
