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
	void	joinCmd( User &user, std::string channelName, std::string key ){
		// Check if channel name is valid
		if (channelName.empty() || channelName[0] != '#') {
			user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :Invalid channel name\r\n");
			return;
		}

		// Remove # from channel name for internal storage
		std::string cleanChannelName = channelName.substr(1);

		// Create new channel if it doesn't exist
		if (!isChannelExist(cleanChannelName)) {
			_channels[cleanChannelName] = new Channel(cleanChannelName, user, key);
			
			// Send JOIN message to user
			user.sendMessage(user.getPrefix() + " JOIN " + channelName + "\r\n");
			
			// Send topic information
			if (_channels[cleanChannelName]->ifTopic()) {
				user.sendMessage(":jarvis_server 332 " + user.getNickname() + " " + channelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n");
			} else {
				user.sendMessage(":jarvis_server 331 " + user.getNickname() + " " + channelName + " :No topic is set\r\n");
			}
			
			// Send names list
			user.sendMessage(":jarvis_server 353 " + user.getNickname() + " = " + channelName + " :" + _channels[cleanChannelName]->usersNames() + "\r\n");
			user.sendMessage(":jarvis_server 366 " + user.getNickname() + " " + channelName + " :End of /NAMES list\r\n");
			
			return;
		}

		// Try to add user to existing channel
		int errCode = _channels[cleanChannelName]->addUser(user, key);
		
		// Handle different error codes
		if (errCode == 473) {
			user.sendMessage(":jarvis_server 473 " + user.getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
			return;
		}
		else if (errCode == 474) {
			user.sendMessage(":jarvis_server 474 " + user.getNickname() + " " + channelName + " :Cannot join channel (banned)\r\n");
			return;
		}
		else if (errCode == 475) {
			user.sendMessage(":jarvis_server 475 " + user.getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
			return;
		}
		else if (errCode == 471) {
			user.sendMessage(":jarvis_server 471 " + user.getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
			return;
		}
		else if (errCode != 0) {
			user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :Cannot join channel\r\n");
			return;
		}

		// Successfully joined - send JOIN message to user
		user.sendMessage(user.getPrefix() + " JOIN " + channelName + "\r\n");
		
		// Broadcast JOIN message to channel
		std::string joinMsg = user.getPrefix() + " JOIN " + channelName + "\r\n";
		_channels[cleanChannelName]->broadcastMsg(user, joinMsg);
		
		// Send topic information
		if (_channels[cleanChannelName]->ifTopic()) {
			user.sendMessage(":jarvis_server 332 " + user.getNickname() + " " + channelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n");
		} else {
			user.sendMessage(":jarvis_server 331 " + user.getNickname() + " " + channelName + " :No topic is set\r\n");
		}
		
		// Send names list
		user.sendMessage(":jarvis_server 353 " + user.getNickname() + " = " + channelName + " :" + _channels[cleanChannelName]->usersNames() + "\r\n");
		user.sendMessage(":jarvis_server 366 " + user.getNickname() + " " + channelName + " :End of /NAMES list\r\n");
	}

	void	partCmd( User &user, std::string channelName, std::string reason ){
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :No such channel\r\n");
			return ;
		}
		
		// Remove # from channel name for internal storage
		std::string cleanChannelName = channelName;
		if (channelName[0] == '#') {
			cleanChannelName = channelName.substr(1);
		}
		
		// Check if user is in the channel
		if (!_channels[cleanChannelName]->isUserInChannel(user.getNickname())) {
			user.sendMessage(":jarvis_server 442 " + user.getNickname() + " " + channelName + " :You're not on that channel\r\n");
			return;
		}
		
		// Broadcast PART message to channel before removing user
		std::string partMsg = user.getPrefix() + " PART " + channelName;
		if (!reason.empty()) {
			partMsg += " :" + reason;
		}
		partMsg += "\r\n";
		_channels[cleanChannelName]->broadcastMsg(user, partMsg);
		
		// Remove user from channel
		_channels[cleanChannelName]->removeUser(user);
		
		// Check if channel is empty and remove it
		if (_channels[cleanChannelName]->getChannelCounter() == 0){
			delete _channels[cleanChannelName];
			_channels.erase(cleanChannelName);
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

	// void	kickCmd( User &user, std::string channelName, std::string toKickUser){
	// 			//check if channel exists
	// 		(void)user;
	// 	if (isChannelExist(channelName) == false){
	// 		// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
	// 		return ;
	// 	}
		
	// }

	// ...existing code...

void infoCmd(User &user, const std::string &channelName = "") {
    // List all channels if no channelName is provided
    if (channelName.empty()) {
        std::ostringstream oss;
        oss << ":jarvis_server INFO :There are " << _channels.size() << " channel(s): ";
        std::map<std::string, Channel*>::iterator it;
        for (it = _channels.begin(); it != _channels.end(); ++it) {
            oss << "#" << it->first << " ";
        }
        oss << "\r\n";
        user.sendMessage(oss.str());
        user.sendMessage(":jarvis_server INFO :To get info about a channel, type: /INFO #channelname\r\n");
        return;
    }

    // Remove # if present
    std::string cleanChannelName = channelName;
    if (channelName[0] == '#')
        cleanChannelName = channelName.substr(1);

    if (!isChannelExist(cleanChannelName)) {
        user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    Channel *chan = _channels[cleanChannelName];
    std::ostringstream info;
    info << ":jarvis_server INFO :Channel #" << cleanChannelName << "\r\n";
    info << ":jarvis_server INFO :Topic: " << (chan->ifTopic() ? chan->getChannelTopic() : "No topic set") << "\r\n";
    info << ":jarvis_server INFO :User count: " << chan->getChannelCounter() << "\r\n";
    info << ":jarvis_server INFO :Users: " << chan->usersNames() << "\r\n";
    user.sendMessage(info.str());
}

// ...existing code...


void listUsersCmd(User &requestingUser) {
    // Check if there are any connected users
    if (_connections.empty()) {
        requestingUser.sendMessage(":jarvis_server 401 " + requestingUser.getNickname() + " :No users are currently connected\r\n");
        return;
    }

    // Build the list of users
    std::ostringstream oss;
    oss << ":jarvis_server USERS :There are " << _connections.size() << " user(s) connected:\r\n";

    for (std::map<int, User*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        User *user = it->second;
        oss << ":jarvis_server USERS :Nickname: " << user->getNickname()
            << ", Username: " << user->getUsername() << "\r\n";
    }

    // Send the list to the requesting user
    requestingUser.sendMessage(oss.str());
}

	bool	isChannelExist( std::string channelName ){
		bool result = _channels.find(channelName) != _channels.end();
		
		// Debug output
		std::cout << "[DEBUG] isChannelExist(" << channelName << ") called" << std::endl;
		std::cout << "[DEBUG] Result: " << (result ? "true" : "false") << std::endl;
		std::cout << "[DEBUG] Available channels: ";
		for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
			std::cout << it->first << " ";
		}
		std::cout << std::endl;
		
		return result;
	}

	// void	HandleErrors( int errorCode, User& user, Channel &channel){
	// 	// if (errorCode == 473) user.sendMessage();
	// }

	// void	sendPrivateMsg( User &reciever, std::string msg ){
	// 	//check if reciever exic
	// }

};