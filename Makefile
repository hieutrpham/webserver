CC = c++ -std=c++20
NAME = webserv
CFLAGS = -Wall -Werror -Wextra -O3
DFLAGS = -MMD -MP

# sources
SRC_PATH = src/
SRC = main.cpp Server.cpp Request.cpp RequestParser.cpp \
	  ConfigParser.cpp ServerConfig.cpp Response.cpp \
	  FileOperation.cpp

#include
INCLUDE = -I./src/

# objects
OBJ = $(SRC:.cpp=.o)
OBJ_PATH = obj/
OBJS = $(addprefix $(OBJ_PATH), $(OBJ))

# dependencies
DEP = $(addprefix $(OBJ_PATH), $(SRC_PATH:.cpp=.d))

all: $(OBJ_PATH) $(NAME)

$(OBJ_PATH)%.o: $(SRC_PATH)%.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(DFLAGS) -o $@ -c $<

$(OBJ_PATH):
	@mkdir -p $(OBJ_PATH)

.SECONDARY: $(OBJS)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

-include $(DEP)

clean:
	rm -rf $(OBJ_PATH)

fclean: clean
	rm -f $(NAME)

re: fclean all

debug:
	$(MAKE) CFLAGS="$(CFLAGS) -ggdb -DDEBUG" re

.PHONY: all clean fclean re debug
