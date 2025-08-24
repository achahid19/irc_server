# Variables
CC = c++

CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS =	$(addprefix srcs/, main.cpp ) \
		$(addprefix srcs/server/, Irc_server.cpp ) \
		$(addprefix srcs/user/, User.cpp ) \
		$(addprefix srcs/utils/, utils.cpp ) \
		$(addprefix srcs/parsing/, Irc_message.cpp ) \
		$(addprefix srcs/channel/, Channel.cpp )

HEADERS = includes/

# Source files
all: jarvis_server

jarvis_server: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -I$(HEADERS) $(SRCS) -o jarvis_server

clean:
	rm -f server

fclean: clean
	rm -f jarvis_server

re: fclean all

.PHONY: all clean

