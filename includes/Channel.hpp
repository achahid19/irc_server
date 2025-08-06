#include "Irc_server.hpp"
#include <unordered_map>
#include <unordered_set>


class Channel
{
private:

	std::string								_channelName;
	std::string								_channelTopic;
	std::unordered_map<std::string, User*>	_channelUsers;
	std::unordered_set<std::string>			_channelOperators;
	std::unordered_set<std::string>			_channelBannedUsers;
	std::string								_channelMode;
	std::string								_channelKey;
	int										_channelMaxUsers;
	bool									_isInviteOnly; // +i
	bool									_isTopicOpera; // +t
	bool									_isKeyReq;	   // +k
	bool									_isLimitSet;   // +l
	bool									_isfull;
	bool									_isTopicSet;
	int										_channelUserCounter;
	std::string								_channelUsersNames;	// for showing the new joiners the list of users on channels

public:

	Channel(std::string channelName, User& opennerUser, std::string key);
	Channel();

	//***	Actions */
	std::string usersNames( void ){
		std::string names;
		for (std::unordered_map<std::string, User*>::iterator it = _channelUsers.begin(); it != _channelUsers.end(); it++){
			if (isOperator(it->first)){
				names.append(it->first + " @opuser ");
			}else{
				names.append(it->first + " ");
			}
		}
		return names;
	}

	int	addUser( User& newUser, std::string key ){
		if (isUserInChannel(newUser.getNickname()) == true){
			//Already in the channel	No error	(you may silently ignore it)
			return ;
		}
		if (_isInviteOnly){
			//!User not invited to +i channel	473	ERR_INVITEONLYCHAN
		}
		if (isUserBanned(newUser.getNickname()) == true){
			//!Banned (bonus feature)	474	ERR_BANNEDFROMCHAN
		}
		if (_isKeyReq && (key != _channelKey))
		{
			//! Wrong key for +k channel	475	ERR_BADCHANNELKEY
		}
		if (_isfull){
			//!Channel is full (+l mode)	471	ERR_CHANNELISFULL
		}
		_channelUsers[newUser.getNickname()] = &newUser;
		_channelUserCounter++;
		if(_channelUserCounter == _channelMaxUsers && _isLimitSet){
			_isfull = true;
		}
		return 0;
		//Notify everyone Send a JOIN message to the channel: :nick!user@host JOIN #channel
		// If channel has a topic, send:332 RPL_TOPIC <nick> #channel :topic text
		// if not 331 RPL_NOTOPIC <nick> #channel :No topic is set
		//  Send the names of users in the channel
		// send userlist
		// 353 RPL_NAMREPLY <nick> = #channel :user1 user2 @opuser
		// 366 RPL_ENDOFNAMES <nick> #channel :End of /NAMES list
	}

	void	removeUser( User &user){
		if (isUserInChannel(user.getNickname()) == false){
			//442 ERR_NOTONCHANNEL <channel> :You're not on that channel
			return ;
		}
		_channelUsers.erase(user.getNickname());
		_channelOperators.erase(user.getNickname());
		_channelUserCounter--;
	}

	void	pingUser( User &user, std::string message ){

		if (isUserInChannel(user.getNickname()) == false){
			//442 ERR_NOTONCHANNEL <channel> :You're not on that channel
			return ;
		}
		if (isUserBanned(user.getNickname()) == true){
			//!Banned (bonus feature)	474	ERR_BANNEDFROMCHAN
			return ;
		}
		broadcastMsg(user, message);
	}

	//check if user in channel
	bool	isUserInChannel( std::string nickname ){
		return _channelUsers.find(nickname) != _channelUsers.end();
	}
	bool	isUserBanned( std::string user ){
		return _channelBannedUsers.find(user) != _channelBannedUsers.end();
	}
	bool	isOperator( std::string user){
		return _channelOperators.find(user) != _channelOperators.end();
	}
	bool	ifTopic( void ){
		return this->_isTopicSet;
	}

	// Actions
	void	broadcastMsg( User& sender, std::string message ){
		for(std::unordered_map<std::string, User*>::iterator it = _channelUsers.begin(); it != _channelUsers.end(); it++){
			if (it->first != sender.getUsername()){
				it->second->sendMessage( message );
			}
		}
	}

	void	setChannelTopic( User &user, std::string newTopic ){
		if (isUserInChannel(user.getNickname()) == false){
			//442 ERR_NOTONCHANNEL <channel> :You're not on that channel
			return ;
		}
		if (isUserBanned(user.getNickname()) == true){
			//!Banned (bonus feature)	474	ERR_BANNEDFROMCHAN
			return ;
		}
		if (isOperator( user.getNickname() ) == false && this->_isTopicOpera == true){
			return ;
		}
		_channelTopic = newTopic;
		//topic succefully changed boread cast
		std::string msg(user.getPrefix() + " TOPIC " + this->_channelName + " :" + newTopic + "!\r\n");
		broadcastMsg( user, msg );
	}



	void	viewTopic( User &user ){
		if (isUserInChannel(user.getNickname()) == false){
			//442 ERR_NOTONCHANNEL <channel> :You're not on that channel
			return ;
		}
		if (isUserBanned(user.getNickname()) == true){
			//!Banned (bonus feature)	474	ERR_BANNEDFROMCHAN
			return ;
		}
		if (_isTopicSet == true){
			//:irc.server 332 <nick> #42 :The topic text
			std::string msg(":irc.server 332" + user.getNickname() + this->_channelName + " :" + this->_channelTopic + "\r\n");
			user.sendMessage(msg);
		}
		else {
			std::string msg(":irc.server 332" + user.getNickname() + this->_channelName + " :No topic is set\r\n");
			user.sendMessage(msg);
		}
	}
	
	
	// modes functions 
	void setInviteOnly(User &user) {
    if (!isOperator(user.getNickname())) {
        // Send error: Only channel operators can set invite-only mode
        return;
    }
    _isInviteOnly = true;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " +i\r\n";
    broadcastMsg(user, msg);
}

void removeInviteOnly(User &user) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isInviteOnly = false;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " -i\r\n";
    broadcastMsg(user, msg);
}

void setTopicOps(User &user) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isTopicOpera = true;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " +t\r\n";
    broadcastMsg(user, msg);
}

void removeTopicOps(User &user) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isTopicOpera = false;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " -t\r\n";
    broadcastMsg(user, msg);
}

	// In Channel.cpp or inline in Channel.hpp:

void setKey(User &user, std::string key) {
    if (!isOperator(user.getNickname())) {
        // Send error: Only channel operators can set channel key
        return;
    }
    _channelKey = key;
    _isKeyReq = true;
    // Broadcast mode change
    std::string msg = user.getPrefix() + " MODE " + _channelName + " +k " + key + "\r\n";
    broadcastMsg(user, msg);
}

void removeKey(User &user, std::string key) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    if (_channelKey == key) {
        _channelKey.clear();
        _isKeyReq = false;
        std::string msg = user.getPrefix() + " MODE " + _channelName + " -k " + key + "\r\n";
        broadcastMsg(user, msg);
    }
}

void setLimit(User &user, std::string limit) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _channelMaxUsers = atoi(limit.c_str());
    _isLimitSet = true;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " +l " + limit + "\r\n";
    broadcastMsg(user, msg);
}

void removeLimit(User &user, std::string limit) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isLimitSet = false;
    _channelMaxUsers = 0;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " -l\r\n";
    broadcastMsg(user, msg);
}

void setOps(User &user, std::string targetUser) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    if (isUserInChannel(targetUser)) {
        _channelOperators.insert(targetUser);
        std::string msg = user.getPrefix() + " MODE " + _channelName + " +o " + targetUser + "\r\n";
        broadcastMsg(user, msg);
    }
}

void removeOps(User &user, std::string targetUser) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _channelOperators.erase(targetUser);
    std::string msg = user.getPrefix() + " MODE " + _channelName + " -o " + targetUser + "\r\n";
    broadcastMsg(user, msg);
}

void setInviteOnly(User &user) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isInviteOnly = true;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " +i\r\n";
    broadcastMsg(user, msg);
}

void removeInviteOnly(User &user) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isInviteOnly = false;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " -i\r\n";
    broadcastMsg(user, msg);
}

void setTopicOps(User &user) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isTopicOpera = true;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " +t\r\n";
    broadcastMsg(user, msg);
}

void removeTopicOps(User &user) {
    if (!isOperator(user.getNickname())) {
        return;
    }
    _isTopicOpera = false;
    std::string msg = user.getPrefix() + " MODE " + _channelName + " -t\r\n";
    broadcastMsg(user, msg);
}







	
	
	void	kickUser( User &user, std::string badUser ){
		if (isUserInChannel(user.getNickname()) == false || isUserInChannel(badUser) == false ){
			//442 ERR_NOTONCHANNEL <channel> :You're not on that channel
			return ;
		}
		if (isUserBanned(user.getNickname()) == true){
			//!Banned (bonus feature)	474	ERR_BANNEDFROMCHAN
			return ;
		}
		if (isOperator( user.getNickname() ) == false){
			return ;
		}
		_channelUsers.erase(badUser);
		_channelOperators.erase(badUser);
		_channelUserCounter--;
		if (_isfull){
			_isfull = false;
		}
	}
	
	
	
	
	//getters
	std::string	getChannelName( void ){
		return this->_channelName;
	}
	std::string getChannelTopic( void ){
		return this->_channelTopic;
	}
	std::string getChannelMode( void ){
		return this->_channelMode;
	}
	std::string getChannelKey( void ){
		return this->_channelKey;
	}
	int			getChannelCounter( void ){
		return this->_channelUserCounter;
	}
};

Channel::Channel(/* args */)
{
}

// first time created empty channel
Channel::Channel(std::string channelName, User& opennerUser, std::string key ) :
_channelName(channelName), _channelUserCounter(1), _isInviteOnly(false),
_isTopicOpera(false), _isKeyReq(false), _isLimitSet(false), _isfull(false)
{
	_channelUsers[opennerUser.getNickname()] = &opennerUser;
	_channelOperators.insert(opennerUser.getNickname());
	if (!key.empty()){
		_channelKey = key;
		_isKeyReq = true;
	}
}

Channel::~Channel()
{
}