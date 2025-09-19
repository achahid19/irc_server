#include "Irc_server.hpp"

void	IrcServer::partCmd( User &user, Irc_message &ircMessage ){

	std::string channelName = ircMessage.getParams()[0];
	if (channelName[0] != '#') return;
	std::string cleanChannelName = channelName.substr(1);
	std::string reason = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";

	if (!isChannelExist(cleanChannelName))
	{
		//:irc.localhost 403 alice #somechan :No such channel
		std::string reply = ":jarvis_server 403 " + user.getNickname() + " #" + cleanChannelName + " :No such channel" + "\r\n";
		user.sendMessage( reply );
		return;
	}

	// Remove # from channel name for internal storage
	// std::string cleanChannelName = channelName;
	// if (channelName[0] == '#') {
	// 	cleanChannelName = channelName.substr(1);
	// }

	// Check if user is in the channel
	// if (!_channels[cleanChannelName]->isUserInChannel(user.getNickname())) {
	// 	user.sendMessage(":jarvis_server 442 " + user.getNickname() + " " + channelName + " :You're not on that channel\r\n");
	// 	return;
	// }

	if (!_channels[cleanChannelName]->isUserInChannel(user.getNickname())) {
		//:irc.localhost 442 alice #somechan :You're not on that channel
		std::string reply = ":jarvis_server 442 " + user.getNickname() + " #" + cleanChannelName + " :You're not on that channel" + "\r\n";
		user.sendMessage( reply );
		return;
	}

	// Broadcast PART message to channel before removing user
	std::string partMsg = user.getPrefix() + " PART " + channelName;
	if (!reason.empty()) {
		partMsg += " :" + reason;
	}
	partMsg += "\r\n";
	_channels[cleanChannelName]->broadcastMsg(user, partMsg);

	// Remove user from channel
	_channels[cleanChannelName]->removeUser(user);

	// Check if channel is empty and remove it
	if (_channels[cleanChannelName]->getChannelCounter() == 0){
		delete _channels[cleanChannelName];
		_channels.erase(cleanChannelName);
	}
}
