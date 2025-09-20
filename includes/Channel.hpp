#pragma once

#include <string>
#include <map>
#include <set>
#include "utils.hpp"

// Forward declaration
class User;

class Channel
{
private:
	std::string								_channelName;
	std::string								_channelTopic;
	std::map<std::string, User*>			_channelUsers;
	std::set<std::string>					_channelOperators;
	std::set<std::string>					_channelBannedUsers;
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
	std::string								_channelUsersNames;
	std::set<std::string>					_inviteList;

public:
	bool 	isInList( std::string nick );
	void	inviteUser(User& user, std::string invitedUser);

	// Constructors and Destructor
	Channel(std::string channelName, User& opennerUser, std::string key);
	Channel();
	~Channel();

	// User management
	std::string usersNames(void);
	int addUser(User& newUser, std::string key);
	void removeUser(User &user);
	void pingUser(User &user, std::string message);

	// Channel checks
	bool isUserInChannel(std::string nickname);
	bool isUserBanned(std::string user);
	bool isOperator(std::string user);
	bool ifTopic(void);

	// Messaging
	void broadcastMsg(User& sender, std::string message);

	// Topic management
	void setChannelTopic(User &user, std::string newTopic);
	void viewTopic(User &user);

	// Channel modes
	void setInviteOnly(User &user);
	void removeInviteOnly(User &user);
	void setTopicOps(User &user);
	void removeTopicOps(User &user);
	void setKey(User &user, std::string key);
	void removeKey(User &user);
	void setLimit(User &user, std::string limit);
	void removeLimit(User &user);
	void setOps(User &user, std::string targetUser);
	void removeOps(User &user, std::string targetUser);
	void availableChannel( void );

	// User management
	void kickUser(User &user, std::string badUser);

	// Getters
	std::string getChannelName(void);
	std::string getChannelTopic(void);
	std::string getChannelMode(void);
	std::string getChannelKey(void);
	int getChannelCounter(void);

	// Mode check methods
	bool isInviteOnly(void) const;
	bool isTopicOps(void) const;
	bool isKeyRequired(void) const;
	bool isLimitSet(void) const;
	int getChannelMaxUsers(void) const;
	bool isfull(void) const;
	std::string WhoIsInvite(void) const;

};