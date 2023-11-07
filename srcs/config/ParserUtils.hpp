#pragma once
#include <cstring>
#include <cstdlib>
#include <string>

size_t	skip_whitespaces(const std::string &line, size_t index);
size_t	expect_char(const std::string &line, size_t index, char expected);
size_t	expect_word_in_range(const std::string &line, size_t index, char start, char end);
void	expect_end_of_content(const std::string &line, size_t index);