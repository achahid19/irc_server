#include "User.hpp"
#include "utils.hpp"
#include "Irc_message.hpp"

// static member initialization
std::set<std::string> User::_registredNicknames = std::set<std::string>();
std::set<std::string> User::_registredUsernames = std::set<std::string>();

// constructor

/**
 * User - User constructor
 *
 * This method will set and initialize User data
 *
 * @param serverPass: the password used by client's to access this server
 * @param sock: the socket of the user connection
 *
 * Return: void.
 */
User::User( const std::string &serverPass, int sock ) {
	this->_userSupportedCommands.insert("NICK");
	this->_userSupportedCommands.insert("USER");
	this->_userSupportedCommands.insert("PASS");
	this->_userSupportedCommands.insert("CAP");
	this->_userSupportedCommands.insert("JOIN");
	this->_userSupportedCommands.insert("PART");
	this->_userSupportedCommands.insert("PRIVMSG");
	this->_userSupportedCommands.insert("TOPIC");
	this->_userSupportedCommands.insert("MODE");
	this->_userSupportedCommands.insert("KICK");
	this->_userSupportedCommands.insert("PING");
	this->_userSupportedCommands.insert("QUIT");

	this->_userData.insert(std::make_pair("NICK", ""));
	this->_userData.insert(std::make_pair("USER", ""));

	// bots commands
	this->_userSupportedCommands.insert("!add");
	this->_userSupportedCommands.insert("!fact");

	this->_serverPassword = serverPass;
	this->_sock = sock;
	this->_regitrationStep = 0;
	this->_state = UNREGISTRED;
};

// methods

/**
 * append - User append method
 *
 * This method will append a buffer to the user's irc message.
 *
 * @param buffer: the buffer to be appended
 *
 * Return: void.
 */
void User::append( const char *buffer ) {
	if (!buffer) return;
	this->_ircMessage.append(buffer);
};

/**
 * registerUser - User registerUser method
 *
 * This method will parse the user's irc message,
 * and register the user if all conditions are met.
 * It will handle NICK, USER, PASS commands,
 * and send appropriate responses to the user.
 *
 * Return: void.
 */
void	User::registerUser( void ) {
	if (this->_ircMessage.empty()) return;
	printMsg(
		"Registering user with:\n" + this->_ircMessage,
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
		if (command == "NICK" && this->getNickname().empty()) {
			if (ircMessage.parseNickCommand() == false) {
				std::string response(
					":jarvis_server 432 " + this->getNickname() + " " + ircMessage.getParams()[0] + " NICK :parameters error\r\n"
				);
				send(this->_sock, response.c_str(), response.size(), 0);
				continue;
			}
			std::string nickName = ircMessage.getParams()[0];
			if (this->setNickname(nickName) == false) {
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
				this->_state = DISCONNECT;
			}
			else if (command == "CAP") {
			if (ircMessage.getParams().size() > 0) {
				std::string subcommand = ircMessage.getParams()[0];
				if (subcommand == "LS") {
					std::string response(":jarvis_server CAP * LS :\r\n");
					send(this->_sock, response.c_str(), response.size(), 0);
				}
				else if (subcommand == "END") {
					std::string response(":jarvis_server CAP * END\r\n");
					send(this->_sock, response.c_str(), response.size(), 0);
				}
			}
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

/**
 * isUserRegistred - User isUserRegistred method
 *
 * This method will check if the user is registered.
 *
 * Return: true if the user is registered, false otherwise.
 */
bool	User::isUserRegistred( void ) const {
	return this->_state == REGISTRED;
}

/**
 * isSupportedCommand - User isSupportedCommand method
 *
 * This method will check if the command is supported by the server.
 *
 * @param cmd: the command to be checked
 *
 * Return: true if the command is supported, false otherwise.
 */
bool	User::isSupportedCommand( std::string const& cmd ) const {
	return (
		this->_userSupportedCommands.find(cmd) != this->_userSupportedCommands.end()
	);
}

/**
 * removeUserNickname - User removeUserNickname method
 *
 * This method will remove the user's nickname from the registered nicknames set.
 * It is called when the user is disconnected or when the nickname is changed.
 *
 * Return: void.
 */
void	User::removeUserNickname( void ) {
	if (this->_userData.find("Nickname") != this->_userData.end()) {
		_registredNicknames.erase(this->_userData["Nickname"]);
	}
};

/**
 * removeUserUsername - User removeUserUsername method
 *
 * This method will remove the user's username from the registered usernames set.
 * It is called when the user is disconnected or when the username is changed.
 *
 * Return: void.
 */
void	User::removeUserUsername( void ) {
	if (this->_userData.find("Username") != this->_userData.end()) {
		_registredUsernames.erase(this->_userData["Username"]);
	}
};

// getters

/**
 * getNickname - User getNickname method
 *
 * This method will return the user's nickname.
 *
 * Return: the user's nickname as a string.
 */
std::string const	User::getNickname( void ) const {
	if (this->_userData.find("Nickname") != this->_userData.end())
		return this->_userData.at("Nickname");
	return "";
};

/**
 * getUsername - User getUsername method
 *
 * This method will return the user's username.
 *
 * Return: the user's username as a string.
 */
std::string const	User::getUsername( void ) const {
	if (this->_userData.find("Username") != this->_userData.end())
		return this->_userData.at("Username");
	return "";
};

/**
 * getState - User getState method
 *
 * This method will return the user's registration state.
 *
 * Return: the user's registration state as an enum user_registration_state.
 */
user_registration_state	User::getState( void ) const {
		return this->_state;
};

// private setters

/**
 * setNickname - User setNickname method
 *
 * This method will set the user's nickname.
 * It checks if the nickname is valid and not already in use.
 * If the nickname is valid, it adds it to the registered nicknames set.
 *
 * @param nickName: the nickname to be set
 *
 * Return: true if the nickname is set successfully, false otherwise.
 */
bool	User::setNickname( std::string const& nickName ) {
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
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"
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

/**
 * _setUsername - User _setUsername method
 *
 * This method will set the user's username.
 * It checks if the username is valid and not already in use.
 * If the username is valid, it adds it to the registered usernames set.
 *
 * @param userName: the username to be set
 *
 * Return: true if the username is set successfully, false otherwise.
 */
bool	User::_setUsername( std::string const& userName ) {
	if (userName.empty() || userName.length() > 9
			|| userName.find_first_not_of(
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"
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

/**
 * _checkPassword - User _checkPassword method
 *
 * This method will check if the provided password matches the server's password.
 *
 * @param password: the password to be checked
 *
 * Return: true if the password is correct, false otherwise.
 */
bool	User::_checkPassword( std::string const& password ) {
	return password == this->_serverPassword;
};
