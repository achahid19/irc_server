#include "Irc_server.hpp"

void	IrcServer::privmsgCmd( User &user, Irc_message &ircMessage ){
	if (ircMessage.getParams().size() > 0) {
		std::string target = ircMessage.getParams()[0];
		std::string msg = ircMessage.getTrailing();


		// DEBUG: Add these lines
	// std::cout << "[DEBUG] PRIVMSG target: '" << target << "'" << std::endl;
	// std::cout << "[DEBUG] PRIVMSG message: '" << msg << "'" << std::endl;
	// std::cout << "[DEBUG] Params size: " << ircMessage.getParams().size() << std::endl;

	// If no trailing, try to get from params
	if (msg.empty() && ircMessage.getParams().size() > 1) {
		msg = ircMessage.getParams()[1];
		// std::cout << "[DEBUG] Using param[1]: '" << msg << "'" << std::endl;
	}

		// If no trailing, try to get from params (fallback for simple messages)
		if (msg.empty() && ircMessage.getParams().size() > 1) {
			msg = ircMessage.getParams()[1];
		}

		if (msg.empty()) {
			user.sendMessage(":jarvis_server 412 " + user.getNickname() + " :No text to send\r\n");
			return;
		}

		if (target[0] == '#') {
			std::string cleanChannelName = target.substr(1);
			if (isChannelExist(cleanChannelName)) {
				// Check if user is in the channel
				if (!_channels[cleanChannelName]->isUserInChannel(user.getNickname())) {
					user.sendMessage(":jarvis_server 404 " + user.getNickname() + " " + target + " :Cannot send to channel\r\n");
					return;
				}

				std::string formattedMsg = user.getPrefix() + " PRIVMSG " + target + " :" + msg + "\r\n";
				_channels[cleanChannelName]->broadcastMsg(user, formattedMsg);
			} else {
				user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + target + " :No such channel\r\n");
			}
		} else {
			// handle private message to user
			// TODO: Implement private messaging between users
			// check if user regersted bason nick
			User* reciever = findUser( target );
			if (reciever != 0){
				std::cout << "tar: " << target << std::endl;
				std::string formattedMsg = user.getPrefix() + " PRIVMSG " + target + " :" + msg + "\r\n";
				reciever->sendMessage(formattedMsg);
			}
			else
				user.sendMessage(":jarvis_server 401 " + user.getNickname() + " " + target + " :No such nick/channel\r\n");
		}
	} else {
		user.sendMessage(":jarvis_server 411 " + user.getNickname() + " :No recipient given (PRIVMSG)\r\n");
	}
}
