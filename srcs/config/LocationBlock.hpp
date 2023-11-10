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

		// parsing
		void	parseBlock(std::ifstream &f);
		void	parseAttribute(const std::string &line, std::ifstream &f);
		int		parseInt(const std::string &line);
		void	parseBool(const std::string &line, bool *dst);
		std::string parsePath(const std::string &line, bool (*is_ok)(char c));
		void	parseCGI(const std::string &line);
		void	parseMethods(const std::string &line);
		void	parseRedirection(const std::string &line);
		void	parseLocation(const std::string &line, std::ifstream &f);
		void	parseError(const std::string &line);
		void	loadError(int http_status, const std::string &path);
		void	parseRoot(const std::string &line);
		void	printState(int indentation) const;
	public:
		LocationBlock(void);
		LocationBlock(const LocationBlock *b, std::ifstream &f);
		~LocationBlock();
		void printConfiguration(int indentation) const;
		// getters
		int getBodySize(void) const;
		bool getAutoIndex(void) const;
		const std::string &getRoot(void) const;
		const std::vector<HTTP::Method> &getMethods(void) const;
		const std::string *getErrorPage(int http_code) const;
		int	getDepth(void) const;
		const LocationBlock *matchLocation(const std::string &path, size_t index) const;
};
