#include "Irc_server.hpp"

/**
 * main - main function for the IRC server
 * 
 * This function will create an instance of IrcServer,
 * and run the server with the specified port and password.
 * 
 * @param argc: the number of command line arguments
 * @param argv: the command line arguments
 * 
 * Return: 0 on success, 1 on failure.
 */
int main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}

	try {
		IrcServer	jarvis_chat_server(atoi(argv[1]), std::string(argv[2]));

		jarvis_chat_server.serverRun();
	}
	catch( const IrcServer::server_error &e ) {
		std::cerr << "Server error: " << e.what() << std::endl;
		return 1;
	}
	catch (const std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
