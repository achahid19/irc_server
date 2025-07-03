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
	std::string							_serverPassword;

	// setter
	void	_setNickname( std::string const& nickName );
	void	_setUsername( std::string const& userName );
	bool	_checkPassword( std::string const& password );
	
public:
	User( const std::string &serverPass, int sock );

	// methods
	void		append( const char *buffer );
	void		registerUser( void );
	bool		isUserRegistred( void ) const;
	bool		isSupportedCommand( std::string const& cmd ) const;

	// getters
	//user_registration_state	getUserState( void ) const;

	std::string const	getNickname( void ) const;
	std::string const	getUsername( void ) const;
};
