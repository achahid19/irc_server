#include "../includes/irc_server.hpp"
#include "../includes/utils.hpp"

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
	_server_password(password)
{
	this->_listeningSocket = -1;
	this->_epollFd = -1;
	this->_maxEvents = 10;
	this->_events.reserve(this->_maxEvents); // reserve space for events
	this->CreateBindListeningSocket();
	this->listenSocket();
	INFO_LOGS && \
	std::cout << Time::getCurrentTime() << " ";
	std::cout << "IrcServer initialized on 127.0.0.1:" << this->_port;
	INFO_LOGS && std::cout << std::endl;
	this->epollCreate();
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
void	IrcServer::CreateBindListeningSocket( void ) {
	this->_listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
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
void	IrcServer::listenSocket( void ) {
	if (listen(this->_listeningSocket, SOMAXCONN) < 0) {
		throw IrcServer::server_error("Failed to listen on socket");
	}
}	

void	IrcServer::epollCreate( void ) {
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
		INFO_LOGS && \
		std::cout << Time::getCurrentTime() << " ";
		std::cout << "Waiting for events..." << std::endl;
		int eventCount = epoll_wait(this->_epollFd, this->_events.data(), this->_maxEvents, -1);
		if (eventCount < 0) {
			ERROR_LOGS && \
			std::cout << Time::getCurrentTime() << " ";
			std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
			continue; // handle error, but continue running the server
		}
		INFO_LOGS && \
		std::cout << Time::getCurrentTime() << " ";
		std::cout << "Ready to handle " << eventCount << " events." << std::endl;
		for (int i = 0; i < eventCount; ++i) {
			if (this->_events[i].data.fd == this->_listeningSocket) {
				// New connection
				int clientSocket = accept(this->_listeningSocket, NULL, NULL);
				if (clientSocket < 0) {
					ERROR_LOGS && std::cerr << "Failed to accept new connection: " << strerror(errno) << std::endl;
					continue;
				}

				struct epoll_event clientEvent;
				clientEvent.events = EPOLLIN | EPOLLET; // Edge-triggered for better performance
				clientEvent.data.fd = clientSocket;

				if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, clientSocket, &clientEvent) < 0) {
					ERROR_LOGS && std::cerr << "Failed to add client socket to epoll: " << strerror(errno) << std::endl;
					close(clientSocket);
					continue;
				}
				this->_events.push_back(clientEvent);
			} else {
				std::cout << "client sent data" << std::endl;
			}
		}
	};
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

