#include <iostream>
#include "config/LocationBlock.hpp"
#include "config/Config.hpp"

LocationBlock::LocationBlock(void): _index(0), _body_size(-1), _auto_index(false), _cgi(""), _root(""), _redirection("")
{
}

LocationBlock::LocationBlock(const LocationBlock *b, std::ifstream &f): _index(0), _cgi(""), _redirection("")
{
	// only these attributes are inherited from parent
	this->_root = b->getRoot();
	this->_methods = b->getMethods();
	this->_body_size = b->getBodySize();
	this->_auto_index = b->getAutoIndex();
	for (std::string line; std::getline(f, line);) {
		++Config::line;
		_index = skip_whitespaces(line, 0);
		if (_index == line.size() || line[_index] == '#' || line[_index] == ';')
			continue ;
		if (line[_index] == '}')
			return ;
		this->parseAttribute(line, f);
	}
	throw RuntimeError("expected '}' but got EOF at line %zu", Config::line);
}

LocationBlock::~LocationBlock()
{
}

void	LocationBlock::parseAttribute(const std::string &line, std::ifstream &f)
{
	if (line.compare(_index, 9, std::string("body_size")) == 0) {
		_index += 9;
		this->parseInt(line, &this->_body_size);
	} else if (line.compare(_index, 8, std::string("redirect")) == 0) {
		_index += 8;
		this->parseRedirection(line);
	} else if (line.compare(_index, 10, std::string("auto_index")) == 0) {
		_index += 10;
		this->parseBool(line, &this->_auto_index);
	} else if (line.compare(_index, 3, std::string("cgi")) == 0) {
		_index += 3;
		this->parseCGI(line);
	} else if (line.compare(_index, 4, std::string("root")) == 0) {
		_index += 4;
		this->parseRoot(line);
	} else if (line.compare(_index, 7, std::string("methods")) == 0) {
		_index += 7;
		this->parseMethods(line);
	// } else if (line.compare(_index, 6, std::string("errors")) == 0) {
	// 	_index += 6;
	// 	this->parseErrors(line);
	} else if (line.compare(_index, 8, std::string("location")) == 0) {
		_index += 8;
		this->parseLocation(line, f);
	} else {
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	}
}

// TODO implement functions
// parsing
void	LocationBlock::parseInt(const std::string &line, int *dst)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	*dst = 0;
	if (!isdigit(line[_index]))
		throw RuntimeError("expected digit at line %zu column %zu", Config::line, _index);
	while (_index < line.size() && isdigit(line[_index])) {
		*dst *= 10;
		*dst += line[_index] - '0';
		++_index;
	}
	expect_end_of_content(line, _index);
}

void	LocationBlock::parseBool(const std::string &line, bool *dst)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	if (line.compare(_index, 4, std::string("true")) == 0) {
		*dst = true;
	} else if (line.compare(_index, 5, std::string("false")) == 0) {
		*dst = false;
	} else {
		throw RuntimeError("unrecognized boolean, should be 'false' or 'true' at line %zu column %zu", Config::line, _index);
	}
	expect_end_of_content(line, _index);
}

void	LocationBlock::parseRoot(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	this->_root = this->parsePath(line, &is_lower_upper);
	if (this->_root[0] != '/') {
		char cwd[1024];
		if (getcwd(cwd, sizeof(cwd)) == NULL)
			throw RuntimeError("failed to access current path");
		this->_root = std::string(cwd) + std::string("/") + this->_root;
	}
}

void	LocationBlock::parseCGI(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	if (_index == line.size())
		throw RuntimeError("unexpected 'newline' at line %zu column %zu", Config::line, _index);
	if (line[_index] != '.')
		throw RuntimeError("cgi should start with '.' at line %zu column %zu", Config::line, _index);
	size_t	start_index = _index++;
	_index = expect_word_in_range(line, _index, 'a', 'z');
	if (_index == start_index + 1)
		throw RuntimeError("cgi should contain letters at line %zu column %zu", Config::line, _index);
	this->_cgi = line.substr(start_index, _index - start_index);
	expect_end_of_content(line, _index);
}

void	LocationBlock::parseMethods(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	while (_index < line.size()) {
		_index = skip_whitespaces(line, _index);
		if (line.compare(_index, 3, "get") == 0) {
			_index += 3;
			this->_methods.push_back(GET);
		} else if (line.compare(_index, 4, "head") == 0) {
			_index += 4;
			this->_methods.push_back(HEAD);
		} else if (line.compare(_index, 4, "post") == 0) {
			_index += 4;
			this->_methods.push_back(POST);
		} else if (line.compare(_index, 5, "delete") == 0) {
			_index += 6;
			this->_methods.push_back(DELETE);
		} else {
			throw RuntimeError("unrecognized http method at line %zu column %zu", Config::line, _index);
		}
		if (line[_index] == ';')
			return ;
		if (!isspace(line[_index]))
			throw RuntimeError("methods should be separated by spaces at line %zu column %zu", Config::line, _index);
	}
	throw RuntimeError("exepcted http method at line %zu column %zu", Config::line, _index);
}

std::string LocationBlock::parsePath(const std::string &line, bool (*is_ok)(char c))
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	bool slash = true;
	size_t start_index = _index;
	while (true) {
		if (_index == line.size())
			throw RuntimeError("unexpected 'newline' at line %zu column %zu", Config::line, _index);
		if (isspace(line[_index]) || line[_index] == ';')
			break ;
		if (is_ok(line[_index])) {
			slash = true;
		} else if (line[_index] == '/') {
			if (slash == false) {
				throw RuntimeError("malformed location path at line %zu column %zu", Config::line, _index);
			}
			slash = false;
		} else {
			throw RuntimeError("illegal character '%c' at line %zu column %zu", line[_index], Config::line, _index);
		}
		++_index;
	}
	return line.substr(start_index, _index - start_index);
}

void	LocationBlock::parseLocation(const std::string &line, std::ifstream &f)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	const std::string location_path = this->parsePath(line, &is_lower);
	expect_char(line, _index, '{');
	LocationBlock *ptr = this;
	this->_locations[location_path] = LocationBlock(ptr, f);
}

void	LocationBlock::parseRedirection(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw RuntimeError("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	size_t start_index = _index;
	if (line.compare(_index, 8, "https://") == 0) {
		_index += 8;
	} else if (line.compare(_index, 7, "http://") == 0) {
		_index += 7;
	} else {
		throw RuntimeError("redirection url should start with 'https:// or 'http://' at line %zu column %zu", Config::line, _index);
	}
	bool sep = false;
	while (_index < line.size()) {
		if (line[_index] == ';')
			break ;
		if (line[_index] == '.') {
			if (sep == false)
				throw RuntimeError("unvalid redirect URL at line %zu column %zu", Config::line, _index);
			sep = false;
		} else if (line[_index] < 'a' || line[_index] > 'z') {
			throw RuntimeError("unvalid redirect URL at line %zu column %zu", Config::line, _index);
		}
		sep = true;
		++_index;
	}
	if (sep == false)
		throw RuntimeError("unvalid redirect URL at line %zu column %zu", Config::line, _index);
	while (_index < line.size()) {
		if (line[_index] == ';')
			break ;
		if (line[_index] == '/') {
			if (sep == false)
				throw RuntimeError("unvalid redirect URL at line %zu column %zu", Config::line, _index);
			sep = false;
		} else if (line[_index] < 'a' || line[_index] > 'z') {
			throw RuntimeError("unvalid redirect URL at line %zu column %zu", Config::line, _index);
		}
		sep = true;
		++_index;
	}
	this->_redirection = line.substr(_index, _index - start_index);
	expect_end_of_content(line, _index);
}

// getters
int LocationBlock::getBodySize(void) const
{
	return this->_body_size;
}

bool LocationBlock::getAutoIndex(void) const
{
	return this->_auto_index;
}

const std::string &LocationBlock::getRoot(void) const
{
	return this->_root;
}

const std::vector<HttpMethod> &LocationBlock::getMethods(void) const
{
	return this->_methods;
}


void LocationBlock::printConfiguration(int indentation) const
{
	std::cout << generate_tabs(indentation) << "Location {" << std::endl;
	++indentation;
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

// const std::map<int, std::string &> &LocationBlock::getErrors(void) const
// {
// 	return this->_errors;
// }
