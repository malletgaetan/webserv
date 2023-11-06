NAME = webserv
CXX = clang++
RM = rm -f
CXXFLAGS = -Wall -Wextra -Werror -I.

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
