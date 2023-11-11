#include "utils.hpp"
#include "Config.hpp"

size_t	skip_whitespaces(const std::string &line, size_t index)
{
	while (index < line.size() && isspace(line[index]))
		++index;
	return index;
}

size_t	expect_char(const std::string &line, size_t index, char expected)
{
	index = skip_whitespaces(line, index);
	if (line[index] == expected)
		return index + 1;
	throw RuntimeError("unrecognized token at line %zu column %zu, expected '%c'", Config::line, index, expected);
}

size_t	expect_word_in_range(const std::string &line, size_t index, char start, char end)
{
	index = skip_whitespaces(line, index);
	while (index < line.size()) {
		if (isspace(line[index]) || line[index] == ';')
			return index;
		if (line[index] < start || line[index] > end)
			throw RuntimeError("unrecognized token at line %zu column %zu, expected range <%c-%c>", Config::line, index, start, end);
		++index;
	}
	throw RuntimeError("unexpected 'newline' at line %zu column %zu", Config::line, index, start, end);
}

void	expect_end_of_content(const std::string &line, size_t index)
{
	index = skip_whitespaces(line, index);
	index = expect_char(line, index, ';');
	if (line.size() != index)
		throw RuntimeError("expected 'newline' at line %zu column %zu", Config::line, index);
}

std::string generate_tabs(int n)
{
    std::string tabs;
    for (int i = 0; i < n; i++) {
        tabs += "\t";
    }
    return tabs;
}

bool	is_uripath(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-');
}

bool	is_filepath(char c)
{
	return (isalnum(c) || c == '.' || c == '_' || c == '-');
}

