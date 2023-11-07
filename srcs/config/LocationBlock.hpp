#pragma once

#include <vector>
#include <map>
#include <fstream>
#include <config/enums.hpp>
#include "config/utils.hpp"

class LocationBlock {
	protected:
		size_t	_index;
		int _body_size;
		bool _auto_index;
		std::string	_cgi;
		std::string _root;
		std::string _redirection;
		std::vector<HttpMethod> _methods;
		std::map <std::string, LocationBlock> _locations;
		// std::map <int, std::string &> _errors;

		// parsing
		void	parseBlock(std::ifstream &f);
		std::string parsePath(const std::string &line);
		void	parseInt(const std::string &line, int *dst);
		void	parseBool(const std::string &line, bool *dst);
		void	parseCGI(const std::string &line);
		void	parseMethods(const std::string &line);
		void	parseRedirection(const std::string &line);
		void	parseLocation(const std::string &line, std::ifstream &f);
		void	parseRoot(const std::string &line);
	public:
		LocationBlock();
		LocationBlock(const LocationBlock *b, std::ifstream &f);
		~LocationBlock();
		void printConfiguration(int indentation) const;
		// getters
		int getBodySize(void) const;
		bool getAutoIndex(void) const;
		const std::string &getRoot(void) const;
		const std::vector<HttpMethod> &getMethods(void) const;
};