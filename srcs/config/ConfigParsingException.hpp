#pragma once
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

class ConfigParsingException: public std::exception {
    public:
        ConfigParsingException(std::string format, ...)
        {
            std::stringstream msg;
            va_list args;

            va_start(args, format);
            size_t i = 0;
            while (true) {
                size_t next_specifier = format.find("%", i);

                if (next_specifier == std::string::npos) {
                    msg << format.substr(i, format.size() - i);
                    break ;
                }
                msg << format.substr(i, next_specifier - i);
                if (format[next_specifier + 1] == 'c') {
                    i = next_specifier + 2;
                    msg << (char)va_arg(args, int);
                } else if (format[next_specifier + 1] == 's') {
                    i = next_specifier + 2;
                    msg << (char *)va_arg(args, char *);
                } else {
                    i = next_specifier + 3;
                    msg << (size_t)va_arg(args, size_t);
                }
            }
            va_end(args);
            _message = msg.str();
        }

        virtual ~ConfigParsingException() throw()
        {
        }

        virtual const char* what() const throw()
        {
            return _message.c_str();
        }
    private:
        std::string _message;
};
