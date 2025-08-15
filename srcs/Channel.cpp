#include "Channel.hpp"
#include "User.hpp"
#include <cstdlib> // for atoi

// Constructors and Destructor
Channel::Channel() {
	_channelName = "";
	_channelTopic = "";
	_channelMode = "";
	_channelKey = "";
	_channelMaxUsers = 0;
	_isInviteOnly = false;
	_isTopicOpera = false;
	_isKeyReq = false;
	_isLimitSet = false;
	_isfull = false;
	_isTopicSet = false;
	_channelUserCounter = 0;
	_channelUsersNames = "";
}

Channel::Channel(std::string channelName, User& opennerUser, std::string key) :
	_channelName(channelName),
	_channelTopic(""),
	_channelMode(""),
	_channelKey(""),
	_channelMaxUsers(0),
	_isInviteOnly(false),
	_isTopicOpera(false),
	_isKeyReq(false),
	_isLimitSet(false),
	_isfull(false),
	_isTopicSet(false),
	_channelUserCounter(1),
	_channelUsersNames("")
{
	_channelUsers[opennerUser.getNickname()] = &opennerUser;
	_channelOperators.insert(opennerUser.getNickname());
	if (!key.empty()) {
		_channelKey = key;
		_isKeyReq = true;
	}
}

Channel::~Channel() {
	// Clean up if needed
}

// User management
std::string Channel::usersNames(void) {
	std::string names;
	for (std::map<std::string, User*>::iterator it = _channelUsers.begin(); it != _channelUsers.end(); it++) {
		if (isOperator(it->first)) {
			names.append(it->first + " @opuser ");
		} else {
			names.append(it->first + " ");
		}
	}
	return names;
}

int Channel::addUser(User& newUser, std::string key) {
	// Check if user is already in channel
	if (isUserInChannel(newUser.getNickname()) == true) {
		// Already in the channel - silently ignore (no error)
		return 0;
	}
	
	// Check invite-only mode
	if (_isInviteOnly) {
		// User not invited to +i channel
		return 473; // ERR_INVITEONLYCHAN
		newUser.sendMessage();
		return;
	}
	
	// Check if user is banned
	if (isUserBanned(newUser.getNickname()) == true) {
		// Banned from channel
		return 474; // ERR_BANNEDFROMCHAN
	}
	
	// Check channel key
	if (_isKeyReq && (key != _channelKey)) {
		// Wrong key for +k channel
		return 475; // ERR_BADCHANNELKEY
	}
	
	// Check if channel is full
	if (_isfull) {
		// Channel is full (+l mode)
		return 471; // ERR_CHANNELISFULL
	}
	
	// All checks passed - add user to channel
	_channelUsers[newUser.getNickname()] = &newUser;
	_channelUserCounter++;
	
	// Update full status if limit is set
	if (_channelUserCounter >= _channelMaxUsers && _isLimitSet) {
		_isfull = true;
	}
	
	return 0; // Success
}

void Channel::removeUser(User &user) {
	if (isUserInChannel(user.getNickname()) == false) {
		// 442 ERR_NOTONCHANNEL <channel> :You're not on that channel
		return;
	}
	_channelUsers.erase(user.getNickname());
	_channelOperators.erase(user.getNickname());
	_channelUserCounter--;
}

void Channel::pingUser(User &user, std::string message) {
	if (isUserInChannel(user.getNickname()) == false) {
		// 442 ERR_NOTONCHANNEL <channel> :You're not on that channel
		return;
	}
	if (isUserBanned(user.getNickname()) == true) {
		// Banned (bonus feature) 474 ERR_BANNEDFROMCHAN
		return;
	}
	broadcastMsg(user, message);
}

// Channel checks
bool Channel::isUserInChannel(std::string nickname) {
	return _channelUsers.find(nickname) != _channelUsers.end();
}

bool Channel::isUserBanned(std::string user) {
	return _channelBannedUsers.find(user) != _channelBannedUsers.end();
}

bool Channel::isOperator(std::string user) {
	return _channelOperators.find(user) != _channelOperators.end();
}

bool Channel::ifTopic(void) {
	return this->_isTopicSet;
}

// Messaging
void Channel::broadcastMsg(User& sender, std::string message) {
	for (std::map<std::string, User*>::iterator it = _channelUsers.begin(); it != _channelUsers.end(); it++) {
		if (it->first != sender.getUsername()) {
			it->second->sendMessage(message);
		}
	}
}

// Topic management
void Channel::setChannelTopic(User &user, std::string newTopic) {
	if (isUserInChannel(user.getNickname()) == false) {
		// 442 ERR_NOTONCHANNEL <channel> :You're not on that channel
		return;
	}
	if (isUserBanned(user.getNickname()) == true) {
		// Banned (bonus feature) 474 ERR_BANNEDFROMCHAN
		return;
	}
	if (isOperator(user.getNickname()) == false && this->_isTopicOpera == true) {
		return;
	}
	_channelTopic = newTopic;
	_isTopicSet = true;
	// topic successfully changed broadcast
	std::string msg(user.getPrefix() + " TOPIC " + this->_channelName + " :" + newTopic + "\r\n");
	broadcastMsg(user, msg);
}

void Channel::viewTopic(User &user) {
	if (isUserInChannel(user.getNickname()) == false) {
		// 442 ERR_NOTONCHANNEL <channel> :You're not on that channel
		return;
	}
	if (isUserBanned(user.getNickname()) == true) {
		// Banned (bonus feature) 474 ERR_BANNEDFROMCHAN
		return;
	}
	if (_isTopicSet == true) {
		// :irc.server 332 <nick> #42 :The topic text
		std::string msg(":irc.server 332 " + user.getNickname() + " #" + this->_channelName + " :" + this->_channelTopic + "\r\n");
		user.sendMessage(msg);
	} else {
		std::string msg(":irc.server 331 " + user.getNickname() + " #" + this->_channelName + " :No topic is set\r\n");
		user.sendMessage(msg);
	}
}

// Channel modes
void Channel::setInviteOnly(User &user) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	_isInviteOnly = true;
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " +i\r\n";
	broadcastMsg(user, msg);
}

void Channel::removeInviteOnly(User &user) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	_isInviteOnly = false;
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " -i\r\n";
	broadcastMsg(user, msg);
}

void Channel::setTopicOps(User &user) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	_isTopicOpera = true;
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " +t\r\n";
	broadcastMsg(user, msg);
}

void Channel::removeTopicOps(User &user) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	_isTopicOpera = false;
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " -t\r\n";
	broadcastMsg(user, msg);
}

void Channel::setKey(User &user, std::string key) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	_channelKey = key;
	_isKeyReq = true;
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " +k " + key + "\r\n";
	broadcastMsg(user, msg);
}

void Channel::removeKey(User &user, std::string key) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	if (_channelKey == key) {
		_channelKey.clear();
		_isKeyReq = false;
		std::string msg = user.getPrefix() + " MODE #" + _channelName + " -k " + key + "\r\n";
		broadcastMsg(user, msg);
	}
}

void Channel::setLimit(User &user, std::string limit) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	_channelMaxUsers = atoi(limit.c_str());
	_isLimitSet = true;
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " +l " + limit + "\r\n";
	broadcastMsg(user, msg);
}

void Channel::removeLimit(User &user, std::string limit) {
	(void)limit; // Suppress unused parameter warning
	if (!isOperator(user.getNickname())) {
		return;
	}
	_isLimitSet = false;
	_channelMaxUsers = 0;
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " -l\r\n";
	broadcastMsg(user, msg);
}

void Channel::setOps(User &user, std::string targetUser) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	if (isUserInChannel(targetUser)) {
		_channelOperators.insert(targetUser);
		std::string msg = user.getPrefix() + " MODE #" + _channelName + " +o " + targetUser + "\r\n";
		broadcastMsg(user, msg);
	}
}

void Channel::removeOps(User &user, std::string targetUser) {
	if (!isOperator(user.getNickname())) {
		return;
	}
	_channelOperators.erase(targetUser);
	std::string msg = user.getPrefix() + " MODE #" + _channelName + " -o " + targetUser + "\r\n";
	broadcastMsg(user, msg);
}

// User management
void Channel::kickUser(User &user, std::string badUser) {
	if (isUserInChannel(user.getNickname()) == false || isUserInChannel(badUser) == false) {
		// 442 ERR_NOTONCHANNEL <channel> :You're not on that channel
		return;
	}
	if (isUserBanned(user.getNickname()) == true) {
		// Banned (bonus feature) 474 ERR_BANNEDFROMCHAN
		return;
	}
	if (isOperator(user.getNickname()) == false) {
		return;
	}
	_channelUsers.erase(badUser);
	_channelOperators.erase(badUser);
	_channelUserCounter--;
	if (_isfull) {
		_isfull = false;
	}
}

// Getters
std::string Channel::getChannelName(void) {
	return this->_channelName;
}

std::string Channel::getChannelTopic(void) {
	return this->_channelTopic;
}

std::string Channel::getChannelMode(void) {
	return this->_channelMode;
}

std::string Channel::getChannelKey(void) {
	return this->_channelKey;
}

int Channel::getChannelCounter(void) {
	return this->_channelUserCounter;
} 