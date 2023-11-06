#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>


// TODO make it subject compliant (no v_start / vsnprintf can be used)
class RuntimeError : public std::exception {
public:
    RuntimeError(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vsnprintf(NULL, 0, format, args);
        va_end(args);

        va_start(args, format);
        size_t size = vsnprintf(NULL, 0, format, args) + 1;
        va_end(args);

        errorMessage = (char*)malloc(size * sizeof(char));

        va_start(args, format);
        vsnprintf(errorMessage, size, format, args);
        va_end(args);
    }

    virtual const char* what() const throw() {
        return errorMessage;
    }

    virtual ~RuntimeError() throw() {
        free(errorMessage);
    }

private:
    char* errorMessage;
};
