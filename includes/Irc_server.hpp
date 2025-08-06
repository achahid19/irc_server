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

#include <map>
#include <vector>
#include <set>

#include "User.hpp"
#include "Channel.hpp"

class IrcServer {
private:
	// server socket
	int						_listeningSocket;
	std::map< int, User* >  _connections; // map client socket to its user data.
	int						_port;
	std::string 			_serverPassword;
	int						_connectionsCount;
	static bool				_running;

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
	std::unordered_map<std::string, Channel*> _channels;

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
	void	joinCmd( User &user, std::string channelName, std::string key ){
		if (isChannelExist(channelName) == false){
			_channels[channelName] = new Channel(channelName, user, key);
			return ;
		}
		int errCode = _channels[channelName]->addUser(user, key);
		if ( errCode ){
			// give back the error code
			return ;
		}
		//is it got add sucssfull ✅
		user.sendMessage(user.getPrefix() + " JOIN #" + _channels[user.getNickname()]->getChannelName() + "\r\n");
		//!			Notify everyone
		//Send a JOIN message to the channel: :nick!user@host JOIN #channel
		//* [:nick!user@host JOIN #channel]
		std::string message(
			user.getPrefix() + " JOIN #" + _channels[user.getNickname()]->getChannelName() + "\r\n");
		_channels[user.getNickname()]->broadcastMsg(user, message);

		//! 		 Send topic
		// If channel has a topic, send
		//* [332 RPL_TOPIC <nick> #channel :topic text]
		//* [331 RPL_NOTOPIC <nick> #channel :No topic is set]
		if(_channels[user.getNickname()]->ifTopic()){
			message.clear();
			message = "332 "  + user.getNickname() + " #" + _channels[user.getNickname()]->getChannelName() + " :topic" +  _channels[user.getNickname()]->getChannelTopic() + "\r\n";
			user.sendMessage(message);
		} else {
			message.clear();
			message = "331 "  + user.getNickname() + " #" + _channels[user.getNickname()]->getChannelName() + " :No topic is set\r\n";
			user.sendMessage(message);
		}

		//		!Send the names of users in the channel
		//* [353 RPL_NAMREPLY <nick> = #channel :user1 user2 @opuser]
		//* [366 RPL_ENDOFNAMES <nick> #channel :End of /NAMES list]
		// Prefix with @ if a user is an operator
		message.clear();
		message = "353 " + user.getNickname() + " #" + _channels[user.getNickname()]->getChannelName() + " :" + _channels[channelName]->usersNames() + "\r\n";
		user.sendMessage(message);
		message.clear();
		message = "356 " + user.getNickname() + " #" + _channels[user.getNickname()]->getChannelName() + " :End of /NAMES list\r\n";
		user.sendMessage(message);
		//:bob!user@host JOIN #chat\r\n


		// If channel has a topic, send:332 RPL_TOPIC <nick> #channel :topic text
		// if not 331 RPL_NOTOPIC <nick> #channel :No topic is set


		// Send the names of users in the channel
		// send userlist
		// 353 RPL_NAMREPLY <nick> = #channel :user1 user2 @opuser
		// 366 RPL_ENDOFNAMES <nick> #channel :End of /NAMES list


	}

	void	partCmd( User &user, std::string channelName, std::string reason ){
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		_channels[channelName]->removeUser(user);
		if ( _channels[channelName]->getChannelCounter() == 0){
			//clear and remove the
			delete _channels[channelName];
			_channels.erase(channelName);
		} else {
			// :nick!user@host PART #channel [:optional message]
			std::string message(
				user.getPrefix() + " PART " + _channels[user.getNickname()]->getChannelName() + reason + "\r\n");
			_channels[user.getNickname()]->broadcastMsg(user, message);
		}
	}

	void	privCmd( User &user, std::string channelName, std::string message ){
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		_channels[channelName]->pingUser( user, message );
	}

	void	topicCmd( User &user, std::string channelName, std::string newTopic)
	{
		//check if channel exists
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		//check if user in channel that will be done at the done inside the channel class
		if ( newTopic.empty() ){
			// viewTopic();
		}
		else {
			_channels[channelName]->setChannelTopic( user, newTopic );
		}
	}

	void	modeCmd( User &user, std::string channelName, std::string mode, std::string param ){
		//check if channel exists
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		if (mode == "+i"){
			_channels[channelName]->setInviteOnly(user);
		}
		else if (mode == "-i"){
			_channels[channelName]->removeInviteOnly(user);
		}
		else if (mode == "+t"){
			_channels[channelName]->setTopicOps(user);
		}	
		else if (mode == "-t"){
			_channels[channelName]->removeTopicOps(user);

		}	
		else if (mode == "+k"){
			_channels[channelName]->setKey(user, param);
		}	
		else if (mode == "-k"){
			_channels[channelName]->removeKey(user, param);

		}	
		else if (mode == "+l"){
			_channels[channelName]->setLimit(user, param);
		}
		else if (mode == "-l"){
			_channels[channelName]->removeLimit(user, param);

		}
		else if (mode == "+o"){
			_channels[channelName]->setOps(user, param);
		}	
		else if (mode == "-o"){
			_channels[channelName]->removeOps(user, param);

		}
		else {}
	}

	void	kickCmd( User &user, std::string channelName, std::string toKickUser){
				//check if channel exists
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		
	}

	bool	isChannelExist( std::string channelName ){
		return _channels.find(channelName) != _channels.end();
	}

	void	HandleErrors( int errorCode, User& user, Channel &channel){
		// if (errorCode == 473) user.sendMessage();
	}

};