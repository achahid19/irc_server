#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

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
		//what evermsge it take it will just structure it
		send(_sock, message.c_str(), message.length(), 0);
	}
	std::string getPrefix() const {
		return ":" + this->getNickname() + "!" + this->getUsername() + "@localhost";
	}
	
};