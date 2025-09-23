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
#include <signal.h> // signal handling

#include <sstream> // for std::ostringstream
#include <map>
#include <vector>
#include <set>

#include "User.hpp"
#include "Channel.hpp"
#include "Irc_message.hpp"
#include "factBot.hpp"

class IrcServer {
private:
	// server socket
	int						_listeningSocket;
	std::map< int, User* >  _connections; // map client socket to its user data.
	int						_port;
	std::string 			_serverPassword;
	int						_connectionsCount;
	static bool				_running;

	// bot
	factBot					*factsBot;

	// epoll stuff
	int									_epollFd;
	std::map<int, struct epoll_event> 	_eventsMap; // map to store events with their client socket
	std::vector<struct epoll_event> 	_events; // events list
	int									_maxEvents;

	std::set<int>						_opennedFds; // find the openFd to remove it.

	// helper private methods
	void	_CreateBindListeningSocket( void );
	void	_listenSocket( void );
	void	_epollCreate( void );
	void	_connectUser( void );
	void	_eventsLoop( int eventsCount );
	void	_handleRequest( int eventIndex, int *bytes_read );

	// signal handler
	static void	signalHandler( int signal );

	// prevent copy or instantiation without params
	IrcServer( IrcServer const &other );
	IrcServer &operator=( IrcServer const &other );
	IrcServer( void );

	//CHA
	std::map<std::string, Channel*> _channels;

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
	class	user_connection_error : public std::exception {
		private:
			const std::string	msg;

		public:
			user_connection_error( const std::string &msg );
			~user_connection_error( void ) throw();
			virtual	const char*	what( void ) const throw();
	};

	//CHA
	void	joinCmd( User &user, std::string channelName, std::string key );
	void	infoCmd(User &user, const std::string &channelName);

// ...existing code...
	void	listUsersCmd(User &requestingUser);
// TODO CHANGE USERS STORED AT A SET
	User	*findUser( std::string nick );
	bool	isChannelExist( std::string channelName );


//Commands Prototype

void	joinCommand(User &user, Irc_message &ircMessage);
void	partCmd( User &user, Irc_message &ircMessage );
void	privmsgCmd( User &user, Irc_message &ircMessage );
void	kickCmd( User &user, Irc_message &ircMessage );
void	topicCmd( User &user, Irc_message &ircMessage );
void	modeCmd( User &user, Irc_message &ircMessage );
void	inviteCmd( User &user, Irc_message &ircMessage);
Channel	*findChannelObj( std::string channelName);
};