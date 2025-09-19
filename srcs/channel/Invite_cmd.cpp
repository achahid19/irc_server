#include "Irc_server.hpp"

void	IrcServer::inviteCmd( User &user, Irc_message &ircMessage){
		if (ircMessage.getParams().size() > 1){
				//check channel is exicst
				std::string channelname = ircMessage.getParams()[1];
				if (channelname[0] != '#') return;
				std::string cleanChannelName = channelname.substr(1);
				if (!isChannelExist(cleanChannelName))
				{
					//:irc.localhost 403 alice #somechan :No such channel
					std::string reply = ":jarvis_server 403 " + user.getNickname() + " #" + cleanChannelName + " :No such channel" + "\r\n";
					user.sendMessage( reply );
					return;
				}
				if (!_channels[cleanChannelName]->isUserInChannel(user.getNickname())) {
					//:irc.localhost 442 alice #somechan :You're not on that channel
					std::string reply = ":jarvis_server 442 " + user.getNickname() + " #" + cleanChannelName + " :You're not on that channel" + "\r\n";
					user.sendMessage( reply );
					return;
				}
				std::string userNick = ircMessage.getParams()[0];
				// add to invite list
				_channels[cleanChannelName]->inviteUser(user, userNick);
				// notify inviter
				user.sendMessage(":jarvis_server 341 " + user.getNickname() + " " + userNick + " #" + cleanChannelName + "\r\n");
				// try to notify invitee if online
				User *invitee = findUser(userNick);
				if (invitee) {
					invitee->sendMessage(user.getPrefix() + " INVITE " + userNick + " #" + cleanChannelName + "\r\n");
				}
			}
	}