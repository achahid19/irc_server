#include "Irc_message.hpp"
#include "utils.hpp"

// Constructor by parameter
Irc_message::Irc_message( const std::string &message ) {
	_message = ::ft_trim_spaces(message);
	this->_prefix = "";
	this->_command = "";
	this->_params.clear();
	this->_trailing = "";
	this->_hasPrefix = false;
}

// methods
void Irc_message::parseMessage( void ) {
	// check for prefix first
	size_t prefixEnd = _message.find(' ');
	if (prefixEnd != std::string::npos && _message[0] == ':') {
		_prefix = ft_trim_spaces(_message.substr(1, prefixEnd - 1)); // skip the ':'
		_message.erase(0, prefixEnd + 1); // remove prefix from message
		this->_hasPrefix = true;
	} else {
		_prefix = "";
	}
	// find command
	size_t commandEnd = _message.find(' ');
	if (commandEnd != std::string::npos) {
		_command = ft_trim_spaces(_message.substr(0, commandEnd));
		_message.erase(0, commandEnd + 1); // remove command from message
	} else {
		_command = _message;
		_message.clear(); // no more message left
	}
	// find params
	size_t paramEnd = _message.find(':');
	if (paramEnd != std::string::npos) {
		// there is a trailing part
		_trailing = ft_trim_spaces(_message.substr(paramEnd + 1));
		_message.erase(paramEnd); // remove trailing part from message
	} else {
		_trailing = "";
	}
	while (!(_message.empty())) {
		size_t spacePos = _message.find(' ');
		if (spacePos != std::string::npos) {
			_params.push_back(ft_trim_spaces(_message.substr(0, spacePos)));
			_message.erase(0, spacePos + 1); // remove param from message
		} else {
			_params.push_back(ft_trim_spaces(_message));
			_message.clear(); // no more message left
		}
	}
}

// parsePassCommand - parse PASS command
bool Irc_message::parsePassCommand( void ) {
	// logic of PASS command, usually used for authentication
	// only one parameter is expected
	// no trailing part
	// PASS <password>
	return _params.size() == 1 && _trailing.empty();
}

// parseNickCommand - parse NICK command
bool Irc_message::parseNickCommand( void ) {
	// logic of NICK command, used to set the nickname
	// only one parameter is expected
	// no trailing part
	// NICK <nickname>
	return _params.size() == 1 && _trailing.empty();
}

// parseUserCommand - parse USER command
bool Irc_message::parseUserCommand( void ) {
	// logic of USER command, used to set the username
	// USER <username> <mode> <unused> :<realname>
	return _params.size() == 3;
}

// hasPrefix - check if message has a prefix
bool Irc_message::hasPrefix( void ) const {
	return _hasPrefix;
}

// get full message for debugging
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
