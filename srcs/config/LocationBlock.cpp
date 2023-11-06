#include "config/LocationBlock.hpp"

LocationBlock::LocationBlock(const Server &s)
{
	// only these attributes are inherited from parent
	this->_root = s.getRoot();
	this->_methods = s.getMethods();
	this->_body_size = s.getBodySize();
	this->_auto_index = s.getAutoIndex();

	for (std::string line; std::getline(fs, line); Config::line++) {
		_index = skip_whitespaces(line, 0);
		if (line[_index] == ';')
			continue ;
		if (line[_index] == '}')
			return ;
		if (line.compare(_index, 9, std:string("body_size")) == 0) {
			_index += 9;
			this->parseInt(line, &this->_body_size);
		} else if (line.compare(_index, 10, std:string("auto_index")) == 0) {
			_index += 10;
			this->parseBool(line, &this->_auto_index);
		} else if (line.compare(_index, 3, std:string("cgi")) == 0) {
			_index += 3;
			this->parseCGI(line);
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

LocationBlock::LocationBlock(const Location &l)
{
	// only these attributes are inherited from parent
	this->_root = s.getRoot();
	this->_methods = s.getMethods();
	this->_body_size = s.getBodySize();
	this->_auto_index = s.getAutoIndex();
}