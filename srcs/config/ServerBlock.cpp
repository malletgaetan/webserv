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
			this->parseInt(line, &this->_listen);
		} else if (line.compare(_index, 11, std::string("server_name")) == 0) {
			_index += 11;
			this->parseServerName(line);
		} else {
			this->parseAttribute(line, f);
		}
	}
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


void ServerBlock::printConfiguration(int indentation) const
{
	(void)indentation;
	std::cout << generate_tabs(indentation) << "Server {" << std::endl;
	++indentation;
	std::cout << generate_tabs(indentation) << "listen " << _listen << std::endl;
	if (_server_name.size() != 0)
		std::cout << generate_tabs(indentation) << "server_name " << _server_name << std::endl;
	if (_body_size != -1)
		std::cout << generate_tabs(indentation) << "body_size " << _body_size << std::endl;
	std::cout << generate_tabs(indentation) << "auto_index " << _auto_index << std::endl;
	if (_cgi.size() != 0)
		std::cout << generate_tabs(indentation) << "cgi " << _cgi << std::endl;
	if (_root.size() != 0)
		std::cout << generate_tabs(indentation) << "root " << _root << std::endl;
	if (_redirection.size() != 0)
		std::cout << generate_tabs(indentation) << "redirection " << _redirection << std::endl;
	if (_methods.size() != 0) {
		std::cout << generate_tabs(indentation) << "methods";
		for (size_t i = 0; i < _methods.size(); i++) {
			std::cout << " ";
			if (_methods[i] == GET) {
				std::cout << "GET";
			} else if (_methods[i] == POST) {
				std::cout << "POST";
			} else if (_methods[i] == DELETE) {
				std::cout << "DELETE";
			} else {
				std::cout << "HEAD";
			}
		}
		std::cout << std::endl;
	}
	std::map<std::string, LocationBlock>::const_iterator it;
    for (it = _locations.begin(); it != _locations.end(); ++it) {
		it->second.printConfiguration(indentation + 1);
    }
	std::cout << generate_tabs(indentation - 1) << "}" << std::endl;
}