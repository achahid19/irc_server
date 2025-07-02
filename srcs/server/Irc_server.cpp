#include "Irc_server.hpp"
#include "utils.hpp"
#include "User.hpp"

/**
 * TODO list -
 * add status for connected users count.
 * disconnect clients when bytes read is 0.
 * Think about implementing parser for syntax commands (PASS, NICK, USER).
 * parse correctly params for NICK, USER, PASS commands.
 * handdle CAP, send no cap to client.
 */

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
}

IrcServer::~IrcServer( void ) {
	this->_listeningSocket >= 0 && close(this->_listeningSocket);
	this->_epollFd >= 0 && close(this->_epollFd);

	INFO_LOGS && \
	std::cout << Time::getCurrentTime() << " - ";
	std::cout << "IrcServer destroyed." << std::endl;
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

void	IrcServer::_epollCreate( void ) {
	this->_epollFd = epoll_create1(0);
	if (this->_epollFd < 0) {
		throw IrcServer::server_error("Failed to create epoll instance");
	}

	struct epoll_event event;
	event.events = EPOLLIN; // listen for incoming connections.
	event.data.fd = this->_listeningSocket;

	if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, this->_listeningSocket, &event) < 0) {
		throw IrcServer::server_error("Failed to add socket to epoll instance");
	}

	this->_events.push_back(event);
}

void	IrcServer::serverRun( void ) {
	while (true) {
		::printMsg("Waiting for events...", INFO_LOGS, COLOR_GRAY);

		int eventsCount = epoll_wait(this->_epollFd, this->_events.data(), this->_maxEvents, -1);
		if (eventsCount < 0) {
			::printMsg(
				"Error in epoll_wait: " + std::string(strerror(errno)),
				ERROR_LOGS,
				COLOR_RED
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

void	IrcServer::_eventsLoop( int eventsCount ) {
	for (int i = 0; i < eventsCount; ++i) {
		if (this->_events[i].data.fd == this->_listeningSocket) {
			try {
				this->_connectUser();
			}
			catch(const user_connection_error& e) {
				::printMsg(e.what(), ERROR_LOGS, COLOR_RED);
			}
		}
		else if(this->_events[i].events & (EPOLLIN | EPOLLET)) {
			int	bytes_read;
			int	clientSocket = this->_events[i].data.fd;
			struct epoll_event event_obj = this->_eventsMap[clientSocket];

			::printMsg("Reading User Socket", INFO_LOGS, COLOR_GRAY);
			this->_readRequest(i, &bytes_read);
			if (bytes_read == 0) {
				// close user stuff
				close(clientSocket);
				this->_opennedFds.erase(clientSocket);
				delete this->_connections[clientSocket];
				this->_connections.erase(clientSocket);
				this->_eventsMap.erase(clientSocket);
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, clientSocket, &event_obj);
			}
		}
	}
}

void	IrcServer::_readRequest( int eventIndex, int *bytes_read ) {
	char			buffer[1024];
	int				bytes;
	int				i = eventIndex;
	int				clientSocket = this->_events[i].data.fd;
	User			*user = this->_connections[clientSocket];

	memset(buffer, 0, sizeof(buffer));
	bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
	if (bytes < 0) {
		::printMsg(
			"Error reading client socket " + ::to_string(clientSocket),
			ERROR_LOGS,
			COLOR_RED
		);
	}
	*bytes_read += bytes;

	user->append(buffer);
	if (!user->isUserRegistred()) {
		user->registerUser();
	}
	else {
		/**
		 * user is already registred,
		 * process some other commands.
		 */
		::printMsg("NON SUPPORTED COMMAND YET !", INFO_LOGS, COLOR_BLUE);
		::printMsg("Received Data:" + std::string(buffer), INFO_LOGS, COLOR_BLUE);
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
