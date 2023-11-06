#include "config/Block.hpp"
#include "config/ParserUtils.hpp"
#include "config/enums.hpp"

// TODO implement functions
// parsing
void	Block::parseInt(const std::string &line, int *dst);
{
	_index = skip_whitespaces(line, _index);
	*dst = 0;
	if (!isdigit(line[index]))
		throw RuntimeError("expected digit at line %zu column %zu", Config::line, _index);
	while (index < line.size() && isdigit(line[index])) {
		*dst += line[index] - '0';
		*dst *= 10;
		++index;
	}
	expect_end_of_content(line, _index);
}

void	Block::parseBool(const std::string &line, bool *dst)
{
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

void	Block::parseRoot(const std::string &line)
{
	this->_root = this->parsePath(line);
	if (this->_root[0] != '/') {
		char cwd[1024];
		if (getcwd(cwd, sizeof(cwd)) == NULL)
			throw RuntimeError("failed to access current path");
		this->_root = std::string(cwd) + std::string("/") + this->_root;
	}
}

void	Block::parseCGI(const std::string &line)
{
	_index = skip_whitespaces(line, _index);
	if (_index == line.size())
		throw RuntimeError("unexpected 'newline' at line %zu column %zu", Config::line, _index);
	if (line[_index] != '.')
		throw RuntimeError("cgi should start with '.' at line %zu column %zu", Config::line, _index);
	size_t	start_index = _index++;
	_index = expect_word_in_range(line, _index, 'a', 'z');
	if (_index == start_index + 1)
		throw RuntimeError("cgi should contain letters at line %zu column %zu", Config::line, _index);
	this->_cgi = line.substr(start_index, _index - start_index - 1);
	expect_end_of_content(line, _index);
}

void	Block::parseMethods(const std::string &line)
{

	while (_index < line.size()) {
		_index = skip_whitespaces(line, _index);
		if (line.compare(_index, 3, "get") == 0) {
			_index += 3;
			this->_methods.push_back(GET);
		}
		else if (line.compare(_index, 4, "head") == 0) {
			_index += 4;
			this->_methods.push_back(HEAD);
		}
		else if (line.compare(_index, 4, "post") == 0) {
			_index += 4;
			this->_methods.push_back(POST);
		}
		else if (line.compare(_index, 5, "delete") == 0) {
			_index += 6;
			this->_methods.push_back(DELETE);
		}
		else {
			throw RuntimeError("unrecognized http method at line %zu column %zu", Config::line, _index);
		}
		if (line[_index] == ';')
			return ;
	}
	throw RuntimeError("exepcted http method at line %zu column %zu", Config::line, _index);
}

const std::string &Block::parsePath(const std::string &line)
{
	_index = skip_whitespaces(line, _index);
	bool slash = true;
	size_t start_index = _index;
	while (true) {
		if (_index == line.size())
			throw RuntimeError("unexpected 'newline' at line %zu column %zu", Config::line, _index);
		if (isspace(line[_index]))
			break column;
		if (line[_index] <= 'z' && line[_index] >= 'a') {
			slash = true;
		} else if (line[_index] == '/') {
			if (slash == false) {
				throw RuntimeError("malformed location path at line %zu column %zu", Config::line, _index);
			}
			slash = false;
		}
		++index;
	}
	return line.substr(start_index, _index - start_index - 1);
}

void	Block::parseLocation(const std::string &line)
{
	const std::string = this->parsePath(line);
	this->_locations[location_path] = LocationBlock(*this);
	expect_end_of_content(line, _index);
}

void	Block::parseRedirection(const std::string &line)
{
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
	this->_redirection = line.substr(_index, _index - start_index - 1);
	expect_end_of_content(line, _index);
}

// TODO add custom error
// void	Block::parseError(const std::string &line)
// {
// 	_index = skip_whitespaces(line, _index);
// 	expect_end_of_content(line, _index);
// }

// getters
int Block::getBodySize(void) const
{
	return this->_body_size;
}

bool Block::getAutoIndex(void) const
{
	return this->_auto_index;
}

const std::string &Block::getCGI(void) const
{
	return this->_cgi;
}

const std::string *Block::getRoot(void) const
{
	return this->_root;
}

const std::vector<std::string &> &Block::getMethods(void) const
{
	return this->_methods;
}

const std::map<int, std::string &> &Block::getErrors(void) const
{
	return this->_errors;
}