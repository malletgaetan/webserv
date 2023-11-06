#pragma once

class Block {
	private:
		size_t	_index;
		int _body_size;
		bool _auto_index;
		std::string	_cgi;
		std::string _root;
		std::vector<std::string &> _methods;
		std::map <std::string, std::string> _locations;
		std::map <int, std::string &> _errors;

		// getters
		int getBodySize(void) const;
		bool getAutoIndex(void) const;
		const std::string &getCGI(void) const;
		const std::string &getRoot(void) const;
		const std::vector<std::string &> &getMethods(void) const;
		const std::map<int, std::string &> &getErrors(void) const;
		// parsing
		void	parseInt(int *dst);
		void	parseBool(bool *dst);
		void	parseAndValidateString(std::string *dst, bool (*validate)(char));
		void	parserMethods(void);
		void	parserLocation(void);
};