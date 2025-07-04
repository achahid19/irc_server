#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <unistd.h>

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

	// unique nickname and username over all users
	static std::set<std::string>		_registredNicknames;
	static std::set<std::string>		_registredUsernames;

	// setter
	bool	_setNickname( std::string const& nickName );
	bool	_setUsername( std::string const& userName );
	bool	_checkPassword( std::string const& password );
	
public:
	User( const std::string &serverPass, int sock );

	// methods
	void		append( const char *buffer );
	void		registerUser( void );
	bool		isUserRegistred( void ) const;
	bool		isSupportedCommand( std::string const& cmd ) const;
	void		removeUserNickname( void ) {
		if (this->_userData.find("Nickname") != this->_userData.end()) {
			_registredNicknames.erase(this->_userData["Nickname"]);
			//this->_userData.erase("Nickname");
		}
	};
	void	removeUserUsername( void ) {
		if (this->_userData.find("Username") != this->_userData.end()) {
			_registredUsernames.erase(this->_userData["Username"]);
			//this->_userData.erase("Username");
		}
	};

	// getters
	std::string const	getNickname( void ) const;
	std::string const	getUsername( void ) const;
	user_registration_state	getState( void ) const {
		return this->_state;
	};


};
