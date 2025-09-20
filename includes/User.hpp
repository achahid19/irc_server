#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include "utils.hpp"

enum user_registration_state {
	UNREGISTRED, // just connected
	REGISTRED,
	DISCONNECT
};

class User {
private:
	std::string							_ircMessage;
	std::map<std::string, std::string>	_userData;
	std::set<std::string>				_userSupportedCommands;
	user_registration_state				_state;
	int									_sock;
	int									_regitrationStep;
	std::string							_serverPassword;

	//* ban flage tacks a ref to channel and she


	// unique nickname and username over all users
	static std::set<std::string>		_registredNicknames;
	static std::set<std::string>		_registredUsernames;

	// private setters
	bool	_setNickname( std::string const& nickName );
	bool	_setUsername( std::string const& userName );
	bool	_checkPassword( std::string const& password );

	// no copy or instantiation without params
	User( User const &other );
	User &operator=( User const &other );
	User( void );

public:
	// constructor
	User( const std::string &serverPass, int sock );

	// methods
	void	append( const char *buffer );
	void	registerUser( void );
	bool	isUserRegistred( void ) const;
	bool	isSupportedCommand( std::string const& cmd ) const;
	void	removeUserNickname( void );
	void	removeUserUsername( void );

	// Debug method to print all attributes
	void printDebugInfo() const;

	// getters
	std::string const		getNickname( void ) const;
	std::string const		getUsername( void ) const;
	user_registration_state	getState( void ) const;

	//CHA
	void sendMessage( std::string message ){
		if (!DEBUG_LOGS){

			std::cout << "[DEBUG] sendMessage called for user: " << getNickname() << std::endl;
			std::cout << "\033[32m" << "[DEBUG] Message: " << message << "\033[32m" << std::endl;
			//std::cout << "[DEBUG] send() returned: " << sent << " for user: " << getNickname() << std::endl;
		}
		send(_sock, message.c_str(), message.length(), 0);
		
	}
	std::string getPrefix() const {
		std::string nickname = this->getNickname();
		std::string username = this->getUsername();

		// Check if nickname and username are set
		if (nickname.empty() || username.empty()) {
			return ":unknown!unknown@localhost";
		}

		return ":" + nickname + "!" + username + "@localhost";
	}

};