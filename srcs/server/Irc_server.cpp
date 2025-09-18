#include "Irc_server.hpp"
#include "utils.hpp"
#include "Irc_message.hpp"
#include "User.hpp"
#include "Channel.hpp"

/**
 * TODO list -
 *
 * make a destructor to close all sockets and free memory. DONE
 * add doc strings for all methods. IN PROGRESS
 * Handle NICK, USER change commands.
 * Handle PRIVMSG command.
 */

bool IrcServer::_running = true; // flag to control server running state, for ctrl+c handling

/**
 * IrcServer - IrcServer constructor
 *
 * This method will set and initialize server data
 *
 * @param port: the port number on which the irc server runs on
 * @param password: the password used by client's to access this server
 *
 * Return: void.
 */
IrcServer::IrcServer( int port, std::string const password )
	: _port(port),
	_serverPassword(password)
{
	this->_listeningSocket = -1;
	this->_epollFd = -1;
	this->_maxEvents = 10;
	this->_events.reserve(this->_maxEvents); // reserve space for events
	this->_CreateBindListeningSocket();
	this->_listenSocket();
	::printMsg(
		"IrcServer Initialized on 127.0.0.1:" + ::to_string(this->_port),
		INFO_LOGS,
		COLOR_GRAY
	);
	this->_epollCreate();
	this->_connectionsCount = 0;
	//BONUS create bot

}

/**
 * ~IrcServer - IrcServer destructor
 *
 * This method will close all opened file descriptors,
 * delete all User objects, and clear the connections map.
 *
 * Return: void.
 */
IrcServer::~IrcServer( void ) {
	// closing opened Fds.
	for (
		std::set<int>::iterator it = this->_opennedFds.begin();
		it != this->_opennedFds.end();
		++it
	) {
		close(*it);
	}
	this->_opennedFds.clear();
	// free user data.
	for (std::map<int, User*>::iterator it = this->_connections.begin();
		it != this->_connections.end();
		++it
	) {
		delete it->second;
	}
	this->_connections.clear();

	printMsg(
		"Jarvis Server Destroyed 🤖",
		INFO_LOGS,
		COLOR_GREEN
	);
}

/**
 * bindSocket - IrcServer bindSocket method
 *
 * This method will create a socket, set the socket options
 * to reuse the address, and bind the socket to localhost
 * and specified port.
 *
 * Return: void.
 */
void	IrcServer::_CreateBindListeningSocket( void ) {
	this->_listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	this->_opennedFds.insert(this->_listeningSocket);
	if (this->_listeningSocket < 0) {
		throw IrcServer::server_error("Failed to create socket");
	}
	struct sockaddr_in serverAddr;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(this->_port);

	int opt = 1;
	if (setsockopt(this->_listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		throw IrcServer::server_error("Failed to set socket options");
	}

	if (bind(this->_listeningSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		throw IrcServer::server_error("Failed to bind socket");
	}
}

/**
 * listenSocket - IrcServer listenSocket method
 *
 * This method will listen on the socket for incoming connections.
 * It sets the maximum number of connections to SOMAXCONN.
 *
 * Return: void.
 */
void	IrcServer::_listenSocket( void ) {
	if (listen(this->_listeningSocket, SOMAXCONN) < 0) {
		throw IrcServer::server_error("Failed to listen on socket");
	}
}

/**
 * _epollCreate - IrcServer epollCreate instance method
 *
 * This method will create an epoll instance,
 * add the listening socket to it, and set up the events.
 *
 * Return: void.
 */
void	IrcServer::_epollCreate( void ) {
	this->_epollFd = epoll_create1(0);
	if (this->_epollFd < 0) {
		throw IrcServer::server_error("Failed to create epoll instance");
	}
	this->_opennedFds.insert(this->_epollFd);

	struct epoll_event event;
	event.events = EPOLLIN; // listen for incoming connections.
	event.data.fd = this->_listeningSocket;

	if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, this->_listeningSocket, &event) < 0) {
		throw IrcServer::server_error("Failed to add socket to epoll instance");
	}

	this->_events.push_back(event);
}

/**
 * serverRun - IrcServer serverRun method
 *
 * This method will run the server, waiting for events
 * and handling them accordingly. It uses epoll to efficiently
 * wait for events on the listening socket and connected clients.
 * It will handle new connections, read data from clients,
 * and process user commands.
 *
 * Return: void.
 */
void	IrcServer::serverRun( void ) {
	signal(SIGINT, this->signalHandler);
	while (_running) {
		::printMsg("Waiting for events...", INFO_LOGS, COLOR_GRAY);

		int eventsCount = epoll_wait(this->_epollFd, this->_events.data(), this->_maxEvents, -1);
		if (eventsCount < 0) {
			::printErr(
				"Error in epoll_wait: " + std::string(strerror(errno))
			);
			continue; // handle error, but continue running the server
		}

		::printMsg(
			"Ready to handle " + ::to_string(eventsCount) + " event(s)",
			INFO_LOGS,
			COLOR_GRAY
		);

		this->_eventsLoop(eventsCount);
	};
}

/**
 * _connectUser - IrcServer connectUser method
 *
 * This method will accept a new user connection,
 * create a new User object for the connection,
 * and add the user to the epoll instance for further events handling.
 * It also updates the connections map and the events map.
 *
 * Return: void.
 */
void	IrcServer::_connectUser( void ) {
	int clientSocket = accept(this->_listeningSocket, NULL, NULL);
	if (clientSocket < 0) {
		throw user_connection_error(
			"Failed to accept new connection: " + std::string(strerror(errno))
		);
	}

	struct epoll_event clientEvent;
	clientEvent.events = EPOLLIN | EPOLLET; // Edge-triggered for better performance
	clientEvent.data.fd = clientSocket;

	if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, clientSocket, &clientEvent) < 0) {
		close(clientSocket);
		throw user_connection_error(
			"Failed to add client socket to epoll: " + std::string(strerror(errno))
		);
	}

	this->_opennedFds.insert(clientSocket);
	this->_connections.insert(std::make_pair(clientSocket, new User(this->_serverPassword, clientSocket)));
	this->_eventsMap.insert(std::make_pair(clientSocket, clientEvent));
}

/**
 * _eventsLoop - IrcServer eventsLoop method
 *
 * This method will loop through the events received from epoll,
 * handling new connections and reading data from connected clients.
 * It will also handle user disconnections and cleanup.
 *
 * @param eventsCount: the number of events to handle
 *
 * Return: void.
 */
void	IrcServer::_eventsLoop( int eventsCount ) {
	for (int i = 0; i < eventsCount; ++i) {
		if (this->_events[i].data.fd == this->_listeningSocket) {
			try {
				this->_connectUser();
				printMsg(
					"New user connected, Number of connections now: " \
					+ ::to_string(++this->_connectionsCount) + " 👁️",
					INFO_LOGS,
					COLOR_GREEN
				);
			}
			catch(const user_connection_error& e) {
				::printErr(e.what());
			}
		}
		else if(this->_events[i].events & (EPOLLIN | EPOLLET)) {
			int	bytes_read = 0;
			int	clientSocket = this->_events[i].data.fd;
			struct epoll_event event_obj = this->_eventsMap[clientSocket];

			::printMsg("Reading User Socket", INFO_LOGS, COLOR_GRAY);
			this->_handleRequest(i, &bytes_read);
			if (bytes_read == 0) {
				// disconnect the user
				User *user = this->_connections[clientSocket];
				std::string name = user->getNickname() != "" ? user->getNickname() : ::to_string(clientSocket);
				printMsg(
					"Closing Connection: See you again " + name + " 👋",
					INFO_LOGS,
					COLOR_GREEN
				);
				close(clientSocket);
				this->_opennedFds.erase(clientSocket);
				this->_eventsMap.erase(clientSocket);
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, clientSocket, &event_obj);
				user->removeUserNickname();
				user->removeUserUsername();
				delete this->_connections[clientSocket];
				this->_connections.erase(clientSocket);
				this->_connectionsCount--;
			}
		}
	}
}

/**
 * _handleRequest - IrcServer handleRequest method
 *
 * This method will read data from the client socket,
 * process the user's commands, and handle user registration.
 * It will also handle disconnections if the user sends a QUIT command.
 *
 * @param eventIndex: the index of the event in the events list
 * @param bytes_read: pointer to an integer to store the number of bytes read
 *
 * Return: void.
 */
void	IrcServer::_handleRequest( int eventIndex, int *bytes_read ) {
	char			buffer[1024];
	int				bytes;
	int				i = eventIndex;
	int				clientSocket = this->_events[i].data.fd;
	User			*user = this->_connections[clientSocket];

	memset(buffer, 0, sizeof(buffer));
	bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
	if (bytes > 0){
		buffer[bytes] = '\0'; // Null-terminate the string
		::printMsg(
			"Received from client " + ::to_string(clientSocket) + ": " + buffer,
			INFO_LOGS,
			COLOR_BLUE
		);
	}
	if (bytes < 0) {

		::printErr(
			"Error reading client socket " + ::to_string(clientSocket)
		);
	}
	*bytes_read += bytes;

	user->append(buffer);
	if (!user->isUserRegistred()) {
		user->registerUser();
		if (user->getState() == DISCONNECT) {
			*bytes_read = 0; // set bytes read to 0 to close the connection
		}
	}
	else {
		/**
		 * user is already registred,
		 * process some other commands.
		 */
		Irc_message ircMessage(buffer);
		ircMessage.parseMessage();

		std::string command = ircMessage.getCommand();
		::printMsg("Processing command: " + command, INFO_LOGS, COLOR_BLUE);

		// Log the parsed IRC message content for every command
		{
			std::string paramsStr;
			const std::vector<std::string> &params = ircMessage.getParams();
			for (std::vector<std::string>::const_iterator it = params.begin(); it != params.end(); ++it) {
				paramsStr += (it == params.begin() ? "" : ", ") + *it;
			}
			std::string trailingStr = ircMessage.getTrailing();
			::printMsg(
				std::string("IRCMessage => CMD: ") + command +
				"\n\t\t\t | PARAMS: [" + paramsStr + "]" +
				"\n\t\t\t | TRAILING: '" + trailingStr + "'",
				INFO_LOGS,
				COLOR_BLUE
			);
		}

		if (command == "QUIT") {
			*bytes_read = 0; // Close connection - cleanup will be handled in _eventsLoop
		}
		else if (command == "JOIN") {
			joinCommand(*user, ircMessage);
			// if (ircMessage.getParams().size() > 0) {
			// 	std::string channelName = ircMessage.getParams()[0];
			// 	std::string key = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";
			// 	joinCmd(*user, channelName, key);
			// } else {
			// 	user->sendMessage(":jarvis_server 461 " + user->getNickname() + " JOIN :Not enough parameters\r\n");
			// }
		}
		else if (command == "PING") {
			if (ircMessage.getParams().size() > 0) {
				std::string response = ":jarvis_server PONG jarvis_server :" + ircMessage.getParams()[0] + "\r\n";
				user->sendMessage(response);
			}
		}
		else if (command == "PART"){

			std::string channelName = ircMessage.getParams()[0];
			std::string reason = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";
			partCmd(*user, channelName.substr(1), reason);
		}
		else if (command == "INFO") {
    if (ircMessage.getParams().size() == 0) {
        infoCmd(*user);
    } else {
        infoCmd(*user, ircMessage.getParams()[0]);
    }
}
		else if (command == "USERS") {
    listUsersCmd(*user);
}
		else if (command == "POPO"){
			user->sendMessage("popopopop\r\n");
				// std::string key = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";

		}
		else if (command == "PRIVMSG") {
			if (ircMessage.getParams().size() > 0) {
				std::string target = ircMessage.getParams()[0];
				std::string msg = ircMessage.getTrailing();


				// DEBUG: Add these lines
			std::cout << "[DEBUG] PRIVMSG target: '" << target << "'" << std::endl;
			std::cout << "[DEBUG] PRIVMSG message: '" << msg << "'" << std::endl;
			std::cout << "[DEBUG] Params size: " << ircMessage.getParams().size() << std::endl;

			// If no trailing, try to get from params
			if (msg.empty() && ircMessage.getParams().size() > 1) {
				msg = ircMessage.getParams()[1];
				std::cout << "[DEBUG] Using param[1]: '" << msg << "'" << std::endl;
			}

				// If no trailing, try to get from params (fallback for simple messages)
				if (msg.empty() && ircMessage.getParams().size() > 1) {
					msg = ircMessage.getParams()[1];
				}

				if (msg.empty()) {
					user->sendMessage(":jarvis_server 412 " + user->getNickname() + " :No text to send\r\n");
					return;
				}

				if (target[0] == '#') {
					std::string cleanChannelName = target.substr(1);
					if (isChannelExist(cleanChannelName)) {
						// Check if user is in the channel
						if (!_channels[cleanChannelName]->isUserInChannel(user->getNickname())) {
							user->sendMessage(":jarvis_server 404 " + user->getNickname() + " " + target + " :Cannot send to channel\r\n");
							return;
						}

						std::string formattedMsg = user->getPrefix() + " PRIVMSG " + target + " :" + msg + "\r\n";
						_channels[cleanChannelName]->broadcastMsg(*user, formattedMsg);
					} else {
						user->sendMessage(":jarvis_server 403 " + user->getNickname() + " " + target + " :No such channel\r\n");
					}
				} else {
					// handle private message to user
					// TODO: Implement private messaging between users
					// check if user regersted bason nick
					User* reciever = findUser( target );
					if (reciever != 0){
						std::cout << "tar: " << target << std::endl;
						std::string formattedMsg = user->getPrefix() + " PRIVMSG " + target + " :" + msg + "\r\n";
						reciever->sendMessage(formattedMsg);
					}
					else
						user->sendMessage(":jarvis_server 401 " + user->getNickname() + " " + target + " :No such nick/channel\r\n");
				}
			} else {
				user->sendMessage(":jarvis_server 411 " + user->getNickname() + " :No recipient given (PRIVMSG)\r\n");
			}
}
		// else if (command == "msg"){
		// 	if (ircMessage.getParams().size() > 0){
		// 		std::string recieverNick = ircMessage.getParams()[1];
		// 		//remove
		// 		std::cout << ircMessage.getParams()[0] << std::endl;
		// 		std::cout << ircMessage.getParams()[1] << std::endl;
		// 	}
		// }
		else if (command == "TOPIC"){
			if (ircMessage.getParams().size() > 0){
				//check channel is exicst
				std::string channelname = ircMessage.getParams()[0];
				if (channelname[0] != '#') return;
				std::string cleanChannelName = channelname.substr(1);
				if (!isChannelExist(cleanChannelName)) {
					//:irc.localhost 403 alice #somechan :No such channel
					std::string reply = ":jarvis_server 403 " + user->getNickname() + " #" + cleanChannelName + " :No such channel" + "\r\n";
					user->sendMessage( reply );
					return;
				}
				if (ircMessage.getTrailing().empty()){
					// return topic
					std::string reply = ":jarvis_server 332 " + user->getNickname() + " #" + cleanChannelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n";
					user->sendMessage( reply );
					return ;
				}
				_channels[cleanChannelName]->setChannelTopic(*user, ircMessage.getTrailing());
				std::string reply = ":jarvis_server 332 " + user->getNickname() + " #" + cleanChannelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n";
				user->sendMessage( reply );

			}
		}
		else if (command == "msg"){
			if (ircMessage.getParams().size() > 0){
				std::string receiverNick = ircMessage.getParams()[0];
				std::string msg = ircMessage.getTrailing();
				User *receiver = findUser( receiverNick );
				( receiver != 0) ? receiver->sendMessage( user->getPrefix() + " PRIVMSG " + receiverNick + " :" + msg + "\r\n" ) : user->sendMessage(":jarvis_server 411 " + user->getNickname() + " :No recipient given (PRIVMSG)\r\n") ;
			}
		}
		//MODE <channel> {[+|-]modes} [parameters...]
		else if (command == "MODE"){
			if (ircMessage.getParams().size() > 1){
				//check channel is exicst
				std::string channelname = ircMessage.getParams()[0];
				if (channelname[0] != '#') return;
				std::string cleanChannelName = channelname.substr(1);
				if (!isChannelExist(cleanChannelName))
				{
					//:irc.localhost 403 alice #somechan :No such channel
					std::string reply = ":jarvis_server 403 " + user->getNickname() + " #" + cleanChannelName + " :No such channel" + "\r\n";
					user->sendMessage( reply );
					return;
				}
				if (!_channels[cleanChannelName]->isUserInChannel(user->getNickname())) {
					//:irc.localhost 442 alice #somechan :You're not on that channel
					std::string reply = ":jarvis_server 442 " + user->getNickname() + " #" + cleanChannelName + " :You're not on that channel" + "\r\n";
					user->sendMessage( reply );
					return;
				}
				//check if the flages are only the allowd alphabitic ilkot   - +locgic start @params[1]
				std::string modeType = ircMessage.getParams()[1];
				if (modeType.size() > 1){
					char sign = modeType[0];
					for (size_t i = 1; i < modeType.size(); i++){
							 if (modeType[i] == 'i' && sign == '+'){
							_channels[cleanChannelName]->setInviteOnly(*user);
						}
						else if (modeType[i] == 'i' && sign == '-'){
							_channels[cleanChannelName]->removeInviteOnly(*user);
						}
						else if (modeType[i] == 't' && sign == '+'){
							_channels[cleanChannelName]->setTopicOps(*user);
						}
						else if (modeType[i] == 't' && sign == '-'){
							_channels[cleanChannelName]->removeTopicOps(*user);
						}
						else if (modeType[i] == 'l' && sign == '+'){
							if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
								std::string limit = ircMessage.getParams()[2];
								_channels[cleanChannelName]->setLimit(*user, limit);
							}
							else {
								//:server 461 <nick> MODE :Not enough parameters
								std::string reply = ":jarvis_server 461 " + user->getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
								user->sendMessage( reply );
								return ;
							}
						}
						else if (modeType[i] == 'l' && sign == '-'){
							_channels[cleanChannelName]->removeLimit(*user);
						}
						else if (modeType[i] == 'k' && sign == '+'){
							if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
								std::string key = ircMessage.getParams()[2];
								_channels[cleanChannelName]->setKey(*user, key);
								std::cout << "***************************\n";
							}
							else {
								//:server 461 <nick> MODE :Not enough parameters
								std::string reply = ":jarvis_server 461 " + user->getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
								user->sendMessage( reply );
								return ;
							}
						}
						else if (modeType[i] == 'k' && sign == '-'){
								_channels[cleanChannelName]->removeKey(*user);
						}
						else if (modeType[i] == 'o' && sign == '+'){
							if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
								std::string newOps = ircMessage.getParams()[2];
								_channels[cleanChannelName]->setOps(*user, newOps);
							}
							else {
								//:server 461 <nick> MODE :Not enough parameters
								std::string reply = ":jarvis_server 461 " + user->getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
								user->sendMessage( reply );
								return ;
							}
						}
						else if (modeType[i] == 'o' && sign == '-'){
							if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
								std::string newOps = ircMessage.getParams()[2];
								_channels[cleanChannelName]->removeOps(*user, newOps);
							}
							else {
								//:server 461 <nick> MODE :Not enough parameters
								std::string reply = ":jarvis_server 461 " + user->getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
								user->sendMessage( reply );
								return ;
							}
						}

					}
				}
			}
		}

		else if (command == "KICK"){
			if (ircMessage.getParams().size() > 1){
				//check channel is exicst
				std::string channelname = ircMessage.getParams()[0];
				if (channelname[0] != '#') return;
				std::string cleanChannelName = channelname.substr(1);
				if (!isChannelExist(cleanChannelName))
				{
					//:irc.localhost 403 alice #somechan :No such channel
					std::string reply = ":jarvis_server 403 " + user->getNickname() + " #" + cleanChannelName + " :No such channel" + "\r\n";
					user->sendMessage( reply );
					return;
				}
				if (!_channels[cleanChannelName]->isUserInChannel(user->getNickname())) {
					//:irc.localhost 442 alice #somechan :You're not on that channel
					std::string reply = ":jarvis_server 442 " + user->getNickname() + " #" + cleanChannelName + " :You're not on that channel" + "\r\n";
					user->sendMessage( reply );
					return;
				}
				std::string userNick = ircMessage.getParams()[1];
				if (!_channels[cleanChannelName]->isUserInChannel(userNick)) {
					// 441 <user> <target> <channel> :They aren't on that channel
					std::string reply = ":jarvis_server 441 " + user->getNickname() + " " + userNick + " #" + cleanChannelName + " :They aren't on that channel" + "\r\n";
					user->sendMessage( reply );
					return;
				}
				_channels[cleanChannelName]->kickUser(*user, userNick);
			}
		}

		else if (command == "INVITE"){
			inviteCmd( *user, ircMessage);
			// if (ircMessage.getParams().size() > 1){
			// 	//check channel is exicst
			// 	std::string channelname = ircMessage.getParams()[1];
			// 	if (channelname[0] != '#') return;
			// 	std::string cleanChannelName = channelname.substr(1);
			// 	if (!isChannelExist(cleanChannelName))
			// 	{
			// 		//:irc.localhost 403 alice #somechan :No such channel
			// 		std::string reply = ":jarvis_server 403 " + user->getNickname() + " #" + cleanChannelName + " :No such channel" + "\r\n";
			// 		user->sendMessage( reply );
			// 		return;
			// 	}
			// 	if (!_channels[cleanChannelName]->isUserInChannel(user->getNickname())) {
			// 		//:irc.localhost 442 alice #somechan :You're not on that channel
			// 		std::string reply = ":jarvis_server 442 " + user->getNickname() + " #" + cleanChannelName + " :You're not on that channel" + "\r\n";
			// 		user->sendMessage( reply );
			// 		return;
			// 	}
			// 	std::string userNick = ircMessage.getParams()[0];
			// 	// add to invite list
			// 	_channels[cleanChannelName]->inviteUser(*user, userNick);
			// 	// notify inviter
			// 	user->sendMessage(":jarvis_server 341 " + user->getNickname() + " " + userNick + " #" + cleanChannelName + "\r\n");
			// 	// try to notify invitee if online
			// 	User *invitee = findUser(userNick);
			// 	if (invitee) {
			// 		invitee->sendMessage(user->getPrefix() + " INVITE " + userNick + " #" + cleanChannelName + "\r\n");
			// 	}
			// }
		}

		else {
			::printMsg("Command not implemented yet: " + command, INFO_LOGS, COLOR_YELLOW);
		}
	}
}



// exception server errors handling classes.

/**
 * server_error - IrcServer server_error constructor
 *
 * This method will set the error message
 *
 * @param msg: the error message to be set
 *
 * Return: void.
 */
IrcServer::server_error::server_error( const std::string &msg )
	: msg(msg) {}

/**
 * ~server_error - IrcServer server_error destructor
 *
 * This method will be called when the server_error object is destroyed.
 * It does not throw any exceptions.
 *
 * Return: void.
 */
IrcServer::server_error::~server_error( void ) throw() {}

/**
 * what - IrcServer server_error what method
 *
 * This method will return the error message
 *
 * Return: const char* - the error message.
 */
const char* IrcServer::server_error::what( void ) const throw() {
	return this->msg.c_str();
}

/**
 * server_error - IrcServer server_error constructor
 *
 * This method will set the error message
 *
 * @param msg: the error message to be set
 *
 * Return: void.
 */
IrcServer::user_connection_error::user_connection_error( const std::string &msg )
	: msg(msg) {}

/**
 * ~server_error - IrcServer server_error destructor
 *
 * This method will be called when the server_error object is destroyed.
 * It does not throw any exceptions.
 *
 * Return: void.
 */
IrcServer::user_connection_error::~user_connection_error( void ) throw() {};

/**
 * what - IrcServer server_error what method
 *
 * This method will return the error message
 *
 * Return: const char* - the error message.
 */
const char* IrcServer::user_connection_error::what( void ) const throw() {
	return this->msg.c_str();
}

/**
 * signalHandler - signal handler for SIGINT
 *
 * @param signal: the signal number (e.g., SIGINT)
 *
 * Return: void.
 */
void	IrcServer::signalHandler( int signal ) {
	(void)signal;
	_running = false;
}
