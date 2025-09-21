#include "Irc_message.hpp"
#include "utils.hpp"

/**
 * Irc_message - Irc_message constructor by parameter
 * 
 * This method will set and initialize Irc_message data
 * 
 * @param message: the message to be parsed
 * 
 * Return: void.
 */
Irc_message::Irc_message( const std::string message ) {
	_message = ::ft_trim_spaces(message);
	this->_prefix = "";
	this->_command = "";
	this->_params.clear();
	this->_trailing = "";
	this->_hasPrefix = false;
}

// methods

/**
 * parseMessage - parse the IRC message
 * 
 * This method will parse the IRC message into its components:
 * prefix, command, parameters, and trailing part.
 * It will also handle the case where there is no prefix.
 * 
 * Return: void.
 */
void Irc_message::parseMessage() {
	// Extract prefix (if it starts with ':')
	if (_message[0] == ':') {
		size_t space = _message.find(' ');
		if (space != std::string::npos) {
			_prefix = ft_trim_spaces(_message.substr(1, space - 1));
			_message.erase(0, space + 1);
			_hasPrefix = true;
		}
	}

	// Extract command
	size_t space = _message.find(' ');
	if (space != std::string::npos) {
		_command = ft_trim_spaces(_message.substr(0, space));
		_message.erase(0, space + 1);
	} else {
		_command = ft_trim_spaces(_message);
		_message.clear();
		return;
	}

	// Extract trailing (starts with " :")
	size_t trailingPos = _message.find(" :");
	if (trailingPos != std::string::npos) {
		_trailing = ft_trim_spaces(_message.substr(trailingPos + 2));
		_message.erase(trailingPos);
	}

	// Extract params (handle quoted args)
	std::vector<std::string> tokens;
	std::string token;
	bool inQuote = false;
	char quoteChar = '\0';

	for (size_t i = 0; i < _message.length(); ++i) {
		char c = _message[i];

		if ((c == '"' || c == '\'') && !inQuote) {
			inQuote = true;
			quoteChar = c;
		}
		else if (c == quoteChar && inQuote) {
			inQuote = false;
		}
		else if (c == ' ' && !inQuote) {
			if (!token.empty()) {
				tokens.push_back(ft_trim_spaces(token));
				token.clear();
			}
		}
		else {
			token += c;
		}
	}
	if (!token.empty())
		tokens.push_back(ft_trim_spaces(token));

	_params = tokens;
}


/* parsePassCommand - parse PASS command
 * 
 * This method will parse the PASS command.
 * It expects only one parameter and no trailing part.
 * PASS <password>
 * 
 * Return: true if the command is valid, false otherwise.
 */
bool Irc_message::parsePassCommand( void ) {
	return _params.size() == 1 && _trailing.empty();
}

/* parseNickCommand - parse NICK command
 * 
 * This method will parse the NICK command.
 * It expects only one parameter and no trailing part.
 * NICK <nickname>
 * 
 * Return: true if the command is valid, false otherwise.
 */
bool Irc_message::parseNickCommand( void ) {
	return _params.size() == 1 && _trailing.empty();
}

/**
 * parseUserCommand - parse USER command
 * 
 * This method will parse the USER command.
 * It expects three parameters and a trailing part.
 * USER <username> <mode> <unused> :<realname>
 * 
 * Return: true if the command is valid, false otherwise.
 */
bool Irc_message::parseUserCommand( void ) {
	return _params.size() == 3;
}

/**
 * hasPrefix - check if the message has a prefix
 * 
 * This method will check if the message has a prefix.
 * It returns true if there is a prefix, false otherwise.
 * 
 * Return: true if the message has a prefix, false otherwise.
 */
bool Irc_message::hasPrefix( void ) const {
	return _hasPrefix;
}

/**
 * clear - clear the message data
 * 
 * This method will clear all the message data,
 * including prefix, command, parameters, and trailing part.
 * It also resets the hasPrefix flag to false.
 * 
 * Return: void.
 */
void	Irc_message::clear( void ) { 
	_message.clear(); 
	_prefix.clear(); 
	_command.clear(); 
	_params.clear(); 
	_trailing.clear(); 
	_hasPrefix = false; 
};

// getters

std::string const& Irc_message::getMessage( void ) const { return _message; };
std::string const& Irc_message::getCommand( void ) const { return _command; };
std::vector<std::string> const& Irc_message::getParams( void ) const { return _params; };
std::string const& Irc_message::getTrailing( void ) const { return _trailing; };

// get full message for debugging

/**
 * getFullMessage - get the full message
 * 
 * This method will return the full message as a string,
 * including the prefix, command, parameters, and trailing part.
 * It formats the message according to IRC protocol.
 * 
 * Return: the full message as a string.
 */
std::string Irc_message::getFullMessage( void ) const {
	std::string fullMessage = _message;
	if (hasPrefix()) {
		fullMessage = ":" + _prefix + " " + fullMessage;
	}
	if (!_command.empty()) {
		fullMessage += _command;
	}
	for (
		std::vector<std::string>::const_iterator it = _params.begin();
		it != _params.end();
		++it
	) {
		fullMessage += " " + *it;
	}
	if (!_trailing.empty()) {
		fullMessage += " :" + _trailing;
	}
	return fullMessage;
}
