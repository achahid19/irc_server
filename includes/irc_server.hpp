#pragma	once

#include <iostream>
#include <cstdlib> // atoi
#include <sys/socket.h> // socket sys call
#include <netinet/in.h> // sockaddr_in structure
#include <arpa/inet.h> // inet_addr function for host
#include <sys/epoll.h> // epoll functions
#include <unistd.h> // close() function
#include <exception>
#include <cstring> // strerror
#include <cerrno> // errno for error handling

#include <map>
#include <vector>

class IrcServer {
private:
	// server socket
	int					_listeningSocket;
	std::map<int, int>  _connections; // map of client sockets to their fds
	int					_port;
	std::string 		_server_password;
	
	// epoll stuff
	int			_epollFd;
	std::map<int, struct epoll_event> _eventsMap; // map to store events with their fd
	std::vector<struct epoll_event> _events; // events list
	int			_maxEvents;

	// helper private methods
	void	CreateBindListeningSocket( void );
	void	listenSocket( void );
	void	epollCreate( void );

	// prevent copy or instantiation without params
	IrcServer( IrcServer const &other );
	IrcServer &operator=( IrcServer const &other );
	IrcServer( void );

public:
	IrcServer( int port, std::string const password );
	~IrcServer( void );

	// Methods
	void	serverRun( void );

	// exception server errors handling classes.
	class	server_error : public std::exception {
		private:
			const std::string	msg;
		public:
			server_error( const std::string &msg );
			~server_error( void ) throw();
			virtual	const char*	what( void ) const throw();
	};
};
