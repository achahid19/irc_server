#include "User.hpp"
#include "utils.hpp"

void User::append( const char *buffer ) {
	if (!buffer) return;
	this->_ircMessage.append(buffer);
};

void	User::registerUser( void ) {
	if (this->_ircMessage.empty()) {
		return;
	}
	printMsg(
		"Registering user " + this->getNickname() + " with message: " + this->_ircMessage,
		DEBUG_LOGS,
		COLOR_CYAN
	);
	while (this->_ircMessage.find("\r\n") != std::string::npos) {
		std::string line = this->_ircMessage.substr(0, this->_ircMessage.find("\r\n"));

		this->_ircMessage.erase(0, this->_ircMessage.find("\r\n") + 2);
		if (line.empty()) continue; // skip empty lines

		std::string command = line.substr(0, line.find(' '));
		if (!this->isSupportedCommand(command)) {
			std::string response(
				":jarvis_server 421 " + this->getNickname() + " " + command + " :Unknown command\r\n"
			);
			send(this->_sock, response.c_str(), response.size(), 0);
			continue; // skip unsupported commands
		}
		if (command == "NICK") {
			std::string nickName = ::ft_trim_spaces(line.substr(line.find(' ') + 1));
			this->_setNickname(nickName);
			this->_regitrationStep++;
		}
		else if (command == "USER") {
			std::string userName = ::ft_trim_spaces(line.substr(line.find(' ') + 1));
			this->_setUsername(userName);
			this->_regitrationStep++;
		}
		else if (command == "PASS") {
			std::string password = ::ft_trim_spaces(line.substr(line.find(' ') + 1));
			if (this->_checkPassword(password)) {
				this->_regitrationStep++;
			} else {
				std::string response(
					":jarvis_server 464 " + this->getNickname() + " :Password incorrect\r\n"
				);
				send(this->_sock, response.c_str(), response.size(), 0);
				// need to be disconnected
			}
		}
	};
	if (this->_regitrationStep == 3) {
		printMsg("User " + this->getNickname() + " registered successfully.", DEBUG_LOGS, COLOR_CYAN);
		this->_state = REGISTRED;
		std::string response(
			":jarvis_server 001 " + this->getNickname() + " :Welcome to the IRC Jarvis Server\r\n"
		);
		send(this->_sock, response.c_str(), response.size(), 0);
		this->_ircMessage.clear();
	}
}
