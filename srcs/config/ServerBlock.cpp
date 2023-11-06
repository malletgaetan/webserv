#include "config/ServerBlock.hpp"
#include "config/ParserUtils.hpp"

ServerBlock::ServerBlock(std::ifstream &fs)
{
	for (std::string line; std::getline(fs, line); Config::line++) {
		_index = skip_whitespaces(line, 0);
		if (line[_index] == ';')
			continue ;
		if (line[_index] == '}')
			return ;
		if (line.compare(_index, 6, std::string("listen")) == 0) {
			_index += 6;
			this->parseInt(line, &this->_listen);
		} else if (line.compare(_index, 9, std:string("body_size")) == 0) {
			_index += 9;
			this->parseInt(line, &this->_body_size);
		} else if (line.compare(_index, 10, std:string("auto_index")) == 0) {
			_index += 10;
			this->parseBool(line, &this->_auto_index);
		} else if (line.compare(_index, 3, std:string("cgi")) == 0) {
			_index += 3;
			this->parseCGI(line);
		} else if (line.compare(_index, 11, std:string("server_name")) == 0) {
			_index += 11;
			this->parseServerName(line);
		} else if (line.compare(_index, 4, std:string("root")) == 0) {
			_index += 4;
			this->parseRoot(&this->_root, &root_validate);
		} else if (line.compare(_index, 7, std:string("methods")) == 0) {
			_index += 7;
			this->parseMethods(line);
		// } else if (line.compare(_index, 6, std:string("errors")) == 0) {
		// 	_index += 6;
		// 	this->parseErrors(line);
		} else if (line.compare(_index, 8, std:string("location")) == 0) {
			_index += 8;
			this->parseLocation(line);
		} else {
			throw RuntimeError("unregonized attribute at line %zu column %zu", Config::line, _index);
		}
	}
	throw RuntimeError("expected '}' but got EOF at line %zu column %zu", Config::line, _index);
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
			slash = true;
		} else if (line[_index] == '.') {
			if (point == false) {
				throw RuntimeError("malformed location path at line %zu column %zu", Config::line, _index);
			}
			slash = false;
		} else {
			throw RuntimeError("uncompatible character in server_name directive at line %zu column %zu", Config::line, _index);
		}
		++index;
	}
	this->_server_name = line.substr(start_index, _index - start_index - 1);
	this->_locations[location_path] = LocationBlock(*this);
}
