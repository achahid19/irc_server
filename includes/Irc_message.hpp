#pragma once

#include <iostream>
#include <vector>

class Irc_message {
private:
	std::string _message;	

	std::string _prefix; // optional
	std::string _command;
	std::vector<std::string> _params;
	std::string _trailing; // optional.

	bool		_hasPrefix;

public:
	// Constructor by parameter
	Irc_message( const std::string &message );

	// methods
	void	parseMessage( void ); // general method to parse the message
	bool	parsePassCommand( void );
	bool	parseNickCommand( void );
	bool	parseUserCommand( void );
	bool	hasPrefix( void ) const;

	// getters
	std::string const& getMessage( void ) const { return _message; };
	std::string const& getCommand( void ) const { return _command; };
	std::vector<std::string> const& getParams( void ) const { return _params; };
	std::string const& getTrailing( void ) const { return _trailing; };

	// get full message
	std::string getFullMessage( void ) const;
};
