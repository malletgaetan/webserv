#include "config/Block.hpp"

// TODO implement functions
// parsing
void	Block::parseInt(int *dst);
void	Block::parseBool(bool *dst);
void	Block::parseAndValidateString(std::string *dst, bool (*validate)(char));
void	Block::parserMethods(void);
void	Block::parserLocation(void);

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