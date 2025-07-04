#include "User.hpp"
#include "utils.hpp"
#include "Irc_message.hpp"

/**
 * to do list -
 * disconnect user if user or nickname is already registred.
 * 
 * check if already regitred for other command.
 */

// static member initialization
std::set<std::string> User::_registredNicknames = std::set<std::string>();
std::set<std::string> User::_registredUsernames = std::set<std::string>();

// constructor 
User::User( const std::string &serverPass, int sock ) {
		this->_userSupportedCommands.insert("NICK");
		this->_userSupportedCommands.insert("USER");
		this->_userSupportedCommands.insert("PASS");
		this->_userSupportedCommands.insert("CAP");

		this->_userData.insert(std::make_pair("NICK", ""));
		this->_userData.insert(std::make_pair("USER", ""));

		this->_serverPassword = serverPass;
		this->_sock = sock;
		this->_regitrationStep = 0;
		this->_state = UNREGISTRED;
};

// methods
void User::append( const char *buffer ) {
	if (!buffer) return;
	this->_ircMessage.append(buffer);
};

void	User::registerUser( void ) {
	if (this->_ircMessage.empty()) return;
	printMsg(
		"Registering user " + this->getNickname() + " with message: " + this->_ircMessage,
		DEBUG_LOGS,
		COLOR_CYAN
	);
	while (this->_ircMessage.find("\r\n") != std::string::npos) {
		std::string line = this->_ircMessage.substr(0, this->_ircMessage.find("\r\n"));
		Irc_message ircMessage(line);

		this->_ircMessage.erase(0, this->_ircMessage.find("\r\n") + 2);
		if (line.empty()) continue; // skip empty lines

		ircMessage.parseMessage();

		std::string command = ircMessage.getCommand();
		if (!this->isSupportedCommand(command)) {
			std::string response(
				":jarvis_server 421 " + this->getNickname() + " " + command + " :Unknown command\r\n"
			);
			send(this->_sock, response.c_str(), response.size(), 0);
			continue; // skip unsupported commands
		}
		if (command == "CAP") {
			std::string response(
				":jarvis_server 421 " + this->getNickname() + " CAP :No CAPs\r\n"
			);
			send(this->_sock, response.c_str(), response.size(), 0);
		}
		else if (command == "NICK" && this->getNickname().empty()) {
			if (ircMessage.parseNickCommand() == false) {
				std::string response(
					":jarvis_server 432 " + this->getNickname() + " " + ircMessage.getParams()[0] + " NICK :parameters error\r\n"
				);
				send(this->_sock, response.c_str(), response.size(), 0);
				continue;
			}
			std::string nickName = ircMessage.getParams()[0];
			if (this->_setNickname(nickName) == false) {
				// nickname is not valid, send error and continue
				continue;
			}
			this->_regitrationStep++;
		}
		else if (command == "USER" && this->getUsername().empty()) {
			if (ircMessage.parseUserCommand() == false) {
				std::string response(
					":jarvis_server 461 " + ircMessage.getParams()[0] + " USER :parameters error\r\n"
				);
				send(this->_sock, response.c_str(), response.size(), 0);
				continue;
			}
			std::string userName = ircMessage.getParams()[0];
			if (!this->_setUsername(userName)) continue;
			this->_regitrationStep++;
		}
		else if (command == "PASS") {
			if (ircMessage.parsePassCommand() == false) {
				std::string response(
					":jarvis_server 461 " + this->getNickname() + " PASS :parameters error\r\n"
				);
				send(this->_sock, response.c_str(), response.size(), 0);
				continue;
			}
			std::string password = ircMessage.getParams()[0];
			if (this->_checkPassword(password)) {
				this->_regitrationStep++;
			} else {
				std::string response(
					":jarvis_server 464 " + this->getNickname() + " :Password incorrect\r\n"
				);
				send(this->_sock, response.c_str(), response.size(), 0);
				// need to be disconnected (close socket)
			}
		}
	};
	if (this->_regitrationStep == 3) {
		printMsg("User " + this->getNickname() + " registered successfully.", DEBUG_LOGS, COLOR_CYAN);
		this->_state = REGISTRED;
		std::string response(
			":jarvis_server 001 " + this->getNickname() + " :Welcome to IRC Jarvis Server 🤖\r\n"
		);
		send(this->_sock, response.c_str(), response.size(), 0);
	}
}

bool	User::isUserRegistred( void ) const {
	return this->_state == REGISTRED;
}

bool	User::isSupportedCommand( std::string const& cmd ) const {
	return (
		this->_userSupportedCommands.find(cmd) != this->_userSupportedCommands.end()
	);
}

void	User::removeUserNickname( void ) {
	if (this->_userData.find("Nickname") != this->_userData.end()) {
		_registredNicknames.erase(this->_userData["Nickname"]);
	}
};

void	User::removeUserUsername( void ) {
	if (this->_userData.find("Username") != this->_userData.end()) {
		_registredUsernames.erase(this->_userData["Username"]);
	}
};

// getters
std::string const	User::getNickname( void ) const {
	if (this->_userData.find("Nickname") != this->_userData.end())
		return this->_userData.at("Nickname");
	return "";
};

std::string const	User::getUsername( void ) const {
	if (this->_userData.find("Username") != this->_userData.end())
		return this->_userData.at("Username");
	return "";
};

user_registration_state	User::getState( void ) const {
		return this->_state;
};

// private setters
bool	User::_setNickname( std::string const& nickName ) {
	if (_registredNicknames.find(nickName) != _registredNicknames.end()) {
		std::string response(
			":jarvis_server 432 * " + nickName + " NICK :is already in use\r\n"
		);
		send(this->_sock, response.c_str(), response.size(), 0);
		this->_state = DISCONNECT;
		return false;
	}
	else if (nickName.empty() || nickName.length() > 9
			|| nickName.find_first_not_of(
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
		) != std::string::npos
	) {
		std::string response(
			":jarvis_server 432 * " + nickName + " NICK :Invalid nickname\r\n"
		);
		send(this->_sock, response.c_str(), response.size(), 0);
		return false;
	}
	this->_userData["Nickname"] = nickName;
	_registredNicknames.insert(nickName);
	return true;
};

bool	User::_setUsername( std::string const& userName ) {
	if (_registredUsernames.find(userName) != _registredUsernames.end()) {
		std::string response(
			":jarvis_server 432 * " + userName + " USER :is already in use\r\n"
		);
		send(this->_sock, response.c_str(), response.size(), 0);
		this->_state = DISCONNECT;
		return false;
	}
	else if (userName.empty() || userName.length() > 9
			|| userName.find_first_not_of(
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
		) != std::string::npos
	) {
		std::string response(
			":jarvis_server 433 " + userName + " USER :Invalid username\r\n"
		);
		send(this->_sock, response.c_str(), response.size(), 0);
		return false;
	}
	this->_userData["Username"] = userName;
	_registredUsernames.insert(userName);
	return true;
};

bool	User::_checkPassword( std::string const& password ) {
	return password == this->_serverPassword;
};
