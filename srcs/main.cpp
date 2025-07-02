#include "Irc_server.hpp"

int main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}

	try {
		IrcServer	jarvis_streaming_server(atoi(argv[1]), std::string(argv[2]));

		jarvis_streaming_server.serverRun();
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
