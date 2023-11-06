#include "ParserUtils.hpp"

size_t	skip_whitespaces(std:string &line, size_t index)
{
	while (index < line.size() && isspace(line[index]))
		++index;
	return index;
}


size_t	expect_char(std::string &line, size_t index, char expected)
{
	index = skip_whitespaces(line, index);
	if (line[index] == expected)
		return index + 1;
	throw RuntimeError("unrecognized token at line %zu column %zu, expected range '%c'", Config::line, index, expected);
}

size_t	expect_word_in_range(std::string &line, size_t index, char start, char end)
{
	index = skip_whitespaces(line, index);
	while (index < line.size()) {
		if (isspace(line[index]))
			return index;
		if (line[index] < start || line[index] > end)
			throw RuntimeError("unrecognized token at line %zu column %zu, expected range <%c-%c>", Config::line, index, start, end);
		++index;
	}
	throw RuntimeError("unrecognized token at line %zu column %zu, expected range <%c-%c>", Config::line, index, start, end);
}