#pragma once

#include "config/Block.hpp"

class LocationBlock: public Block {
	private:
		int _body_size;
		bool _auto_index;
		std::string _cgi;
		std::string _redirection;
		std::string _root;
		std::vector<std::string &> _methods;
		std::map <std::string, std::string> _locations;
		std::map <int, std::string &> _errors;
	public:
		LocationBlock(const Server &s);
		LocationBlock(const Location &l);
		~LocationBlock();
};