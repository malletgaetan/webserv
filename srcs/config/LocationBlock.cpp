#include <iostream>
#include <string>
#include "config/LocationBlock.hpp"
#include "config/Config.hpp"

// PUBLIC

LocationBlock::LocationBlock(void):  _index(0), _body_limit(0), _auto_index(false)
{
	_methods.push_back(HTTP::GET);
}

LocationBlock::LocationBlock(const LocationBlock *b, const std::string &location, std::ifstream &f): _index(0), _body_limit(b->_body_limit), _auto_index(b->_auto_index), _cgi_extension(b->_cgi_extension), _cgi_path(b->_cgi_path), _root(b->_root), _location(location), _methods(b->_methods)
{
	// only these attributes are inherited from parent
	for (std::string line; std::getline(f, line);) {
		++Config::line;
		_index = skip_whitespaces(line, 0);
		if (_index == line.size() || line[_index] == '#' || line[_index] == ';')
			continue ;
		if (line[_index] == '}')
			return ;
		_parseAttribute(line, f);
	}
	throw ConfigParsingException("expected '}' but got EOF at line %zu", Config::line);
}

LocationBlock::~LocationBlock()
{
}

void LocationBlock::printConfiguration(int indentation) const
{
	std::cout << generate_tabs(indentation) << "Location " << _location << " {" << std::endl;
	_printState(indentation + 1);
	std::cout << generate_tabs(indentation) << "}" << std::endl;
}

const std::string &LocationBlock::getCGIExecutable(void) const
{
	return _cgi_path;
}

bool LocationBlock::isCGI(const std::string &str) const
{
	if (_cgi_extension.size() == 0)
		return false;
	size_t last_point = str.find_last_of('.');
	if (last_point == std::string::npos)
		return false;
	return ((_cgi_extension.size() == str.size() - last_point) && (str.compare(last_point, str.size() - last_point, _cgi_extension) == 0));
}

const std::string &LocationBlock::getErrorPage(int http_code) const
{
	std::map<int, std::string>::const_iterator it = _errors.find(http_code);
	if (it == _errors.end())
		return HTTP::default_error(http_code);
	return it->second;
}

const std::string &LocationBlock::getRedirection(void) const
{
	return _redirection;
}

bool LocationBlock::isRedirect(void) const
{
	return _redirection.size() != 0;
}

const std::string &LocationBlock::getIndex(void) const
{
	return _index_str;
}

std::pair<size_t, const LocationBlock *> LocationBlock::matchLocation(const std::string &path, size_t index) const
{
	std::map<std::string, LocationBlock>::const_iterator it;
	std::pair<size_t, const LocationBlock *> ret = std::pair<size_t, const LocationBlock *>(index, this);

	for (it = _locations.begin(); it != _locations.end(); ++it) {
		if (it->first.size() == 1 || path.compare(index, it->first.size(), it->first) == 0) {
			if (path.size() == index + it->first.size()) // total match
				return std::pair<size_t, const LocationBlock *>(index + it->first.size(), &(it->second));
			std::pair<size_t, const LocationBlock *> tmp = it->second.matchLocation(path, index + it->first.size() - (int)(it->first.size() == 1));
			if (tmp.first > ret.first)
				ret = tmp;
		}
	}
	return ret;
}


bool LocationBlock::isUnauthorizedMethod(HTTP::Method method) const
{
	for (size_t i = 0; i < _methods.size(); ++i) {
		if (_methods[i] == method)
			return false;
	}
	return true;
}

const std::string LocationBlock::getFilepath(const std::string &path) const
{
	if (path[path.size() - 1] == '/') {
		const std::string diff = path.substr(_location.size(), path.size() - _location.size());
		return join_path(join_path(_root, diff), _index_str);
	}
	return join_path(_root, path.substr(_location.size(), path.size() - _location.size()));
}

int LocationBlock::getBodyLimit(void) const
{
	return _body_limit;
}

bool LocationBlock::isAutoIndex(void) const
{
	return _auto_index;
}

const std::string &LocationBlock::getRoot(void) const
{
	return _root;
}

const std::vector<HTTP::Method> &LocationBlock::getMethods(void) const
{
	return _methods;
}

// PRIVATE

void	LocationBlock::_parseAttribute(const std::string &line, std::ifstream &f)
{
	if (line.compare(_index, 10, std::string("body_limit")) == 0) {
		_index += 10;
		_body_limit = _parseInt(line);
		expect_end_of_content(line, _index);
	} else if (line.compare(_index, 8, std::string("redirect")) == 0) {
		_index += 8;
		_parseRedirection(line);
	} else if (line.compare(_index, 10, std::string("auto_index")) == 0) {
		_index += 10;
		_parseBool(line, &_auto_index);
	} else if (line.compare(_index, 3, std::string("cgi")) == 0) {
		_index += 3;
		_parseCGI(line);
	} else if (line.compare(_index, 4, std::string("root")) == 0) {
		_index += 4;
		_parseRoot(line);
	 } else if (line.compare(_index, 5, std::string("index")) == 0) {
		_index += 5;
		_parseIndex(line);
	} else if (line.compare(_index, 7, std::string("methods")) == 0) {
		_index += 7;
		_methods.clear(); // don't apply inherited methods
		_parseMethods(line);
	} else if (line.compare(_index, 5, std::string("error")) == 0) {
		_index += 5;
		_parseError(line);
	} else if (line.compare(_index, 8, std::string("location")) == 0) {
		_index += 8;
		_parseLocation(line, f);
	} else {
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	}
}

int	LocationBlock::_parseInt(const std::string &line)
{
	int	res = 0;
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line, line[_index]);
	_index = skip_whitespaces(line, _index);
	if (!isdigit(line[_index]))
		throw ConfigParsingException("expected digit at line %zu column %zu", Config::line, _index);
	while (_index < line.size() && isdigit(line[_index])) {
		res *= 10;
		res += line[_index] - '0';
		++_index;
	}
	return res;
}

void	LocationBlock::_parseBool(const std::string &line, bool *dst)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	if (line.compare(_index, 4, std::string("true")) == 0) {
		_index += 4;
		*dst = true;
	} else if (line.compare(_index, 5, std::string("false")) == 0) {
		_index += 5;
		*dst = false;
	} else {
		throw ConfigParsingException("unrecognized boolean, should be 'false' or 'true' at line %zu column %zu", Config::line, _index);
	}
	expect_end_of_content(line, _index);
}

void	LocationBlock::_parseRoot(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	_root = _parsePath(line, &is_filepath);
	if (_root.size() == 0)
		throw ConfigParsingException("missing root value at line %zu column %zu", Config::line, _index);
}

void	LocationBlock::_parseIndex(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	_index_str = _parsePath(line, &is_filepath);
	expect_end_of_content(line, _index);
}

void	LocationBlock::_parseCGI(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	if (_index == line.size())
		throw ConfigParsingException("unexpected 'newline' at line %zu column %zu", Config::line, _index);
	if (line[_index] != '.')
		throw ConfigParsingException("cgi should start with '.' at line %zu column %zu", Config::line, _index);
	size_t	start_index = _index++;
	_index = expect_word_in_range(line, _index, 'a', 'z');
	if (_index == start_index + 1)
		throw ConfigParsingException("cgi should contain letters at line %zu column %zu", Config::line, _index);
	_cgi_extension = line.substr(start_index, _index - start_index);
	_cgi_path = _parsePath(line, &is_filepath);
	expect_end_of_content(line, _index);
}

void	LocationBlock::_parseMethods(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	_methods.clear();
	while (_index < line.size()) {
		_index = skip_whitespaces(line, _index);
		if (line.compare(_index, 3, "get") == 0) {
			_index += 3;
			_methods.push_back(HTTP::GET);
		} else if (line.compare(_index, 4, "head") == 0) {
			_index += 4;
			_methods.push_back(HTTP::HEAD);
		} else if (line.compare(_index, 4, "post") == 0) {
			_index += 4;
			_methods.push_back(HTTP::POST);
		} else if (line.compare(_index, 6, "delete") == 0) {
			_index += 6;
			_methods.push_back(HTTP::DELETE);
		} else if (line.compare(_index, 3, "put") == 0) {
			_index += 3;
			_methods.push_back(HTTP::PUT);
		} else {
			throw ConfigParsingException("unrecognized http method at line %zu column %zu", Config::line, _index);
		}
		if (line[_index] == ';')
			return ;
		if (!isspace(line[_index]))
			throw ConfigParsingException("methods should be separated by spaces at line %zu column %zu", Config::line, _index);
	}
	throw ConfigParsingException("exepcted http method at line %zu column %zu", Config::line, _index);
}

std::string LocationBlock::_parsePath(const std::string &line, bool (*is_ok)(char c))
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	bool slash = true;
	size_t start_index = _index;
	while (true) {
		if (_index == line.size())
			throw ConfigParsingException("unexpected 'newline' at line %zu column %zu", Config::line, _index);
		if (isspace(line[_index]) || line[_index] == ';')
			break ;
		if (is_ok(line[_index])) {
			slash = true;
		} else if (line[_index] == '/') {
			if (slash == false)
				throw ConfigParsingException("malformed location path at line %zu column %zu", Config::line, _index);
			slash = false;
		} else {
			throw ConfigParsingException("illegal character '%c' at line %zu column %zu", line[_index], Config::line, _index);
		}
		++_index;
	}
	return line.substr(start_index, _index - start_index);
}

void	LocationBlock::_parseLocation(const std::string &line, std::ifstream &f)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	const std::string location_path = _parsePath(line, &is_uripath);
	if (location_path[0] != '/')
		throw ConfigParsingException("location path should start with a '/' at line %zu column %zu", Config::line, _index);
	expect_char(line, _index, '{');
	if (_locations.find(location_path) != _locations.end())
		throw ConfigParsingException("non-unique location path detected at line %zu", Config::line);
	_locations[location_path] = LocationBlock((LocationBlock *)this, join_path(_location, location_path), f);
}

void	LocationBlock::_parseRedirection(const std::string &line)
{
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("unrecognized attribute at line %zu", Config::line);
	_index = skip_whitespaces(line, _index);
	size_t start_index = _index;
	if (line.compare(_index, 8, "https://") == 0) {
		_index += 8;
	} else if (line.compare(_index, 7, "http://") == 0) {
		_index += 7;
	} else {
		throw ConfigParsingException("redirection url should start with 'https:// or 'http://' at line %zu column %zu", Config::line, _index);
	}
	bool sep = false;
	while (_index < line.size()) {
		if (line[_index] == ';')
			break ;
		if (line[_index] == '.' || line[_index] == '/') {
			if (sep == false)
				throw ConfigParsingException("unvalid redirect URL at line %zu column %zu", Config::line, _index);
			sep = false;
		} else if (line[_index] == ':') {
			while (isdigit(line[++_index]))
				;
			continue ;
		} else if (line[_index] < 'a' || line[_index] > 'z') {
			throw ConfigParsingException("unvalid redirect URL at line %zu column %zu", Config::line, _index);
		}
		++_index;
		sep = true;
	}
	_redirection = line.substr(start_index, _index - start_index);
	expect_end_of_content(line, _index);
}

void	LocationBlock::_parseError(const std::string &line)
{
	int http_error = _parseInt(line);
	if (_index == line.size() || !isspace(line[_index]))
		throw ConfigParsingException("expected filepath after error code at line %zu column %zu", Config::line, _index);
	std::string errorpath = _parsePath(line, &is_filepath);
	if (errorpath.size() < 6)
		throw ConfigParsingException("invalid filepath for error code at line %zu column %zu", Config::line, _index);
	if (errorpath.substr(errorpath.size() - 5) != ".html") // TODO: maybe this is a dumb rule, could be easily removed
		throw ConfigParsingException("invalid file for error code, should be html, at line %zu column %zu", Config::line, _index);
	expect_end_of_content(line, _index);
	_loadError(http_error, errorpath);
}

void	LocationBlock::_loadError(int http_status, const std::string &path)
{
	std::ifstream file(path.c_str());
    std::string content;
    if (!file)
		throw ConfigParsingException("failed to open error page %s at line %zu", path.c_str(), Config::line);
	file.seekg(0, std::ios::end);
	content.resize(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(&content[0], content.size());
	file.close();
	_errors[http_status] = content;
}

void LocationBlock::_printState(int indentation) const
{
	if (_body_limit != 0)
		std::cout << generate_tabs(indentation) << "body_limit " << _body_limit << std::endl;
	std::cout << generate_tabs(indentation) << "auto_index " << _auto_index << std::endl;
	if (_cgi_extension.size() != 0)
		std::cout << generate_tabs(indentation) << "cgi " << _cgi_extension << " " << _cgi_path << std::endl;
	if (_index_str.size() != 0)
		std::cout << generate_tabs(indentation) << "index " << _index_str << std::endl;
	if (_root.size() != 0)
		std::cout << generate_tabs(indentation) << "root " << _root << std::endl;
	if (_redirection.size() != 0)
		std::cout << generate_tabs(indentation) << "redirection " << _redirection << std::endl;
	if (_methods.size() != 0) {
		std::cout << generate_tabs(indentation) << "methods";
		for (size_t i = 0; i < _methods.size(); i++) {
			std::cout << " ";
			if (_methods[i] == HTTP::GET) {
				std::cout << "GET";
			} else if (_methods[i] == HTTP::POST) {
				std::cout << "POST";
			} else if (_methods[i] == HTTP::DELETE) {
				std::cout << "DELETE";
			} else if (_methods[i] == HTTP::PUT){
				std::cout << "PUT";
			} else {
				std::cout << "HEAD";
			}
		}
		std::cout << std::endl;
	}
	if (_errors.size() != 0) {
		std::map<int, std::string>::const_iterator it;
		for (it = _errors.begin(); it != _errors.end(); ++it) {
			std::cout << generate_tabs(indentation) << "errors " << it->first << std::endl;
		}
	}

	std::map<std::string, LocationBlock>::const_iterator it;
    for (it = _locations.begin(); it != _locations.end(); ++it) {
		it->second.printConfiguration(indentation + 1);
    }
}

