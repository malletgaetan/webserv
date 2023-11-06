#pragma once
#include <cstring>
#include <cstdlib>

size_t	skip_whitespaces(std:string &line, size_t index);
size_t	expect_char(std::string &line, size_t index, char expected);
size_t	expect_word_in_range(std::string &line, size_t index, char start, char end);
bool	cgi_validate(char c);
bool	host_validate(char c);