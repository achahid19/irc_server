#include "Irc_server.hpp"
#include "utils.hpp"
#include "Irc_message.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "factBot.hpp"

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
		"IrcServer Initialized on 0.0.0.0:" + ::to_string(this->_port),
		INFO_LOGS,
		COLOR_GRAY
	);
	this->_epollCreate();
	this->_connectionsCount = 0;
	this->factsBot = new factBot("facts.txt");
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
	delete this->factsBot;
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
	serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
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
			::printErr("Error in epoll_wait");
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

		// bot commands
		if (command == "!add") {
			std::string response;
			// check params count
			if (ircMessage.getParams().size() != 2) {
				response = ":jarvis_server " + user->getNickname() + " :Invalid param: Usage: !add <key> <fact>\r\n";
				send(clientSocket, response.c_str(), response.size(), 0);
				return ;
			}
			response = ":jarvis_server " + user->getNickname() + " :Your fact is stored Successfully!\r\n";
			if (this->factsBot->getFact(ircMessage.getParams()[0]) != "") {
				response = ":jarvis_server " + user->getNickname() + " :Fact with this key already exists. Use a different key or update the fact.\r\n";
				send(clientSocket, response.c_str(), response.size(), 0);
				return ;
			}	
			this->factsBot->addFact(ircMessage.getParams()[0], ircMessage.getParams()[1]);
			send(clientSocket, response.c_str(), response.size(), 0);
		}
		else if (command == "!fact") {
			std::string response;
			// check params count
			if (ircMessage.getParams().size() != 1) {
				response = ":jarvis_server " + user->getNickname() + " :Invalid param: Usage: !fact <key>\r\n";
				send(clientSocket, response.c_str(), response.size(), 0);
				return ;
			}
			std::string fact = this->factsBot->getFact(ircMessage.getParams()[0]);
			if (fact != "") {
				response = ":jarvis_server " + user->getNickname() + " :" + fact + "\r\n";
			} else {
				response = ":jarvis_server " + user->getNickname() + " :No fact found for key: " + ircMessage.getParams()[0] + "\r\n";
			}
			send(clientSocket, response.c_str(), response.size(), 0);
		}
		else if (command == "QUIT") {
			*bytes_read = 0; // Close connection - cleanup will be handled in _eventsLoop
		}
		else if (command == "JOIN") {
			joinCommand(*user, ircMessage);
		}
		else if (command == "PART"){
			partCmd(*user, ircMessage);
		}
		else if (command == "PING") {
			if (ircMessage.getParams().size() > 0) {
				std::string response = ":jarvis_server PONG jarvis_server :" + ircMessage.getParams()[0] + "\r\n";
				user->sendMessage(response);
			}
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
			privmsgCmd( *user, ircMessage );
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
			topicCmd( *user, ircMessage );
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
			modeCmd( *user, ircMessage );
		}

		else if (command == "KICK"){
			kickCmd(*user, ircMessage);
		}

		else if (command == "INVITE"){
			inviteCmd( *user, ircMessage);
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
