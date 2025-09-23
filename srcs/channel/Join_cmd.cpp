#include "Irc_server.hpp"

void IrcServer::joinCommand(User &user, Irc_message &ircMessage){
	if (ircMessage.getParams().size() > 0) {
		std::string channelName = ircMessage.getParams()[0];
		std::string key = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";
		// joinCmd(user, channelName, key);




		// Check if channel name is valid
		if (channelName.empty() || channelName[0] != '#') {
			user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :Invalid channel name\r\n");
			return;
		}

		// Remove # from channel name for internal storage
		std::string cleanChannelName = channelName.substr(1);

		// Create new channel if it doesn't exist
		if (!isChannelExist(cleanChannelName)) {
			_channels[cleanChannelName] = new Channel(cleanChannelName, user, key);

			// Send JOIN message to user
			user.sendMessage(user.getPrefix() + " JOIN " + channelName + "\r\n");

			// Send topic information
			if (_channels[cleanChannelName]->ifTopic()) {
				user.sendMessage(":jarvis_server 332 " + user.getNickname() + " " + channelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n");
			} else {
				user.sendMessage(":jarvis_server 331 " + user.getNickname() + " " + channelName + " :No topic is set\r\n");
			}

			// Send names list
			user.sendMessage(":jarvis_server 353 " + user.getNickname() + " = " + channelName + " :" + _channels[cleanChannelName]->usersNames() + "\r\n");
			user.sendMessage(":jarvis_server 366 " + user.getNickname() + " " + channelName + " :End of /NAMES list\r\n");

			return;
		}

		// if channel is sitted on inivete mode and the user nick name is on the invited list
		// chane is inviet channel and the usern not invited
		if (_channels[cleanChannelName]->isInviteOnly() && !_channels[cleanChannelName]->isInList(user.getNickname())){
			// std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1 " << std::endl;
			user.sendMessage(":jarvis_server 473 " + user.getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
			return ;
		}


		// Try to add user to existing channel
		int errCode = _channels[cleanChannelName]->addUser(user, key);

		// Handle different error codes
		if (errCode == 473) {
			user.sendMessage(":jarvis_server 473 " + user.getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
			return;
		}
		else if (errCode == 474) {
			user.sendMessage(":jarvis_server 474 " + user.getNickname() + " " + channelName + " :Cannot join channel (banned)\r\n");
			return;
		}
		else if (errCode == 475) {
			user.sendMessage(":jarvis_server 475 " + user.getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
			return;
		}
		else if (errCode == 471) {
			user.sendMessage(":jarvis_server 471 " + user.getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
			return;
		}
		else if (errCode != 0) {
			user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :Cannot join channel\r\n");
			return;
		}

		// Successfully joined - send JOIN message to user
		user.sendMessage(user.getPrefix() + " JOIN " + channelName + "\r\n");

		// Broadcast JOIN message to channel
		std::string joinMsg = user.getPrefix() + " JOIN " + channelName + "\r\n";
		_channels[cleanChannelName]->broadcastMsg(user, joinMsg);

		// Send topic information
		if (_channels[cleanChannelName]->ifTopic()) {
			user.sendMessage(":jarvis_server 332 " + user.getNickname() + " " + channelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n");
		} else {
			user.sendMessage(":jarvis_server 331 " + user.getNickname() + " " + channelName + " :No topic is set\r\n");
		}

		// Send names list
		user.sendMessage(":jarvis_server 353 " + user.getNickname() + " = " + channelName + " :" + _channels[cleanChannelName]->usersNames() + "\r\n");
		user.sendMessage(":jarvis_server 366 " + user.getNickname() + " " + channelName + " :End of /NAMES list\r\n");












	} else {
		user.sendMessage(":jarvis_server 461 " + user.getNickname() + " JOIN :Not enough parameters\r\n");
	}
}