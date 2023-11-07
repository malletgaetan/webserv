NAME = webserv
CXX = g++
RM = rm -f
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -I ./srcs

SRCS = $(shell find ./srcs -name '*.cpp')
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean $(NAME)

.PHONY:			all clean fclean re
