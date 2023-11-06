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
			this->parseInt(&this->_listen);
		} else if (line.compare(_index, 9, std:string("body_size")) == 0) {
			_index += 9;
			this->parseInt(&this->_body_size);
		} else if (line.compare(_index, 10, std:string("auto__index")) == 0) {
			_index += 10;
			this->parseBool(&this->_auto_index);
		} else if (line.compare(_index, 3, std:string("cgi")) == 0) {
			_index += 3;
			this->parseAndValidateString(&this->_cgi, &cgi_validate);
		} else if (line.compare(_index, 4, std:string("host")) == 0) {
			_index += 4;
			this->parseAndValidateString(&this->_host, &host_validate);
		} else if (line.compare(_index, 11, std:string("server_name")) == 0) {
			_index += 11;
			this->parseAndValidateString(&this->_server_name, &server_name_validate);
		} else if (line.compare(_index, 4, std:string("root")) == 0) {
			_index += 4;
			this->parseAndValidateString(&this->_root, &root_validate);
		} else if (line.compare(_index, 7, std:string("methods")) == 0) {
			_index += 7;
			this->parseMethods();
		} else if (line.compare(_index, 6, std:string("errors")) == 0) {
			_index += 6;
			this->parseErrors();
		} else if (line.compare(_index, 8, std:string("location")) == 0) {
			_index += 8;
			this->parseLocation();
		} else {
			throw RuntimeError("unregonized attribute at line %zu column %zu", Config::line, _index);
		}
	}
	throw RuntimeError("expected '}' but got EOF");
}
