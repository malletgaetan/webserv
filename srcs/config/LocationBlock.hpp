#pragma once

#include <vector>
#include <map>
#include <fstream>
#include "http.hpp"
#include "config/utils.hpp"

class LocationBlock {
	protected:
		int		_depth;
		size_t	_index;
		int _body_limit;
		bool _auto_index;
		std::string	_cgi_extension;
		std::string	_cgi_path;
		std::string _root;
		std::string _redirection;
		std::vector<HTTP::Method> _methods;
		std::map<std::string, LocationBlock> _locations;
		std::map<int, std::string> _errors;

		void	_parseBlock(std::ifstream &f);
		void	_parseAttribute(const std::string &line, std::ifstream &f);
		int		_parseInt(const std::string &line);
		void	_parseBool(const std::string &line, bool *dst);
		std::string _parsePath(const std::string &line, bool (*is_ok)(char c));
		void	_parseCGI(const std::string &line);
		void	_parseIndex(const std::string &line);
		void	_parseMethods(const std::string &line);
		void	_parseRedirection(const std::string &line);
		void	_parseLocation(const std::string &line, std::ifstream &f);
		void	_parseError(const std::string &line);
		void	_loadError(int http_status, const std::string &path);
		void	_parseRoot(const std::string &line);
		void	_printState(int indentation) const;
	public:
		LocationBlock(void);
		LocationBlock(const LocationBlock *b, std::ifstream &f);
		~LocationBlock();
		void printConfiguration(int indentation) const;
		int getBodySize(void) const;
		bool getAutoIndex(void) const;
		const std::string &getRoot(void) const;
		const std::vector<HTTP::Method> &getMethods(void) const;
		const std::string *getErrorPage(int http_code) const;
		int	getDepth(void) const;
		const LocationBlock *matchLocation(const std::string &path, size_t index) const;
};
