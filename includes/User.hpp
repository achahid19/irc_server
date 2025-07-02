#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>

// enum reading_status {
// 	READING_ERROR,
// 	COMPLETED,
// 	NOT_SUPPORTED_CMD,
// 	UNCOMPLETED,
// };

enum user_registration_state {
	UNREGISTRED, // just connected
	REGISTRED,
};

class User {
private:
	std::string							_ircMessage;
	std::map<std::string, std::string>	_userData;
	std::set<std::string>				_userSupportedCommands;
	user_registration_state				_state;
	int									_sock;
	int									_regitrationStep;

	// std::string				_nickName;
	// std::string				_userName;
	std::string				_serverPassword;

	// helper methods

	// setter
	void	_setNickname( std::string const& nickName ) {
		this->_userData["Nickname"] = nickName;
	};
	void	_setUsername( std::string const& userName ) {
		this->_userData["Username"] = userName;
	};
	bool	_checkPassword( std::string const& password ) {
		return password == this->_serverPassword;
	};
	
public:
	User( const std::string &serverPass, int sock ) : _state(UNREGISTRED) {
		this->_userSupportedCommands.insert("NICK");
		this->_userSupportedCommands.insert("USER");
		this->_userSupportedCommands.insert("PASS");

		this->_userData.insert(std::make_pair("NICK", ""));
		this->_userData.insert(std::make_pair("USER", ""));

		this->_serverPassword = serverPass;
		this->_sock = sock;
		this->_regitrationStep = 0;
	};

	// methods
	void		append( const char *buffer );
	void		registerUser( void );
	bool		isUserRegistred( void ) const {
		return this->_state == REGISTRED;
	}
	bool		isSupportedCommand( std::string const& cmd ) const {
		if (this->_userSupportedCommands.find(cmd) != this->_userSupportedCommands.end()) {
			return true;
		}
		return false;
	}

	// getters
	user_registration_state	getUserState( void ) const {
		return this->_state;
	}

	std::string const	getNickname( void ) const {
		if (this->_userData.find("Nickname") != this->_userData.end())
			return this->_userData.at("Nickname");
		return "";
	};
	std::string const	getUsername( void ) const {
		if (this->_userData.find("Username") != this->_userData.end())
			return this->_userData.at("Username");
		return "";
	};
};
