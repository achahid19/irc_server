#include "Irc_server.hpp"

void	IrcServer::topicCmd( User &user, Irc_message &ircMessage )
	{
		//check if channel exists
		// if (isChannelExist(channelName) == false){
		// 	// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
		// 	return ;
		// }
		// //check if user in channel that will be done at the done inside the channel class
		// if ( newTopic.empty() ){
		// 	// viewTopic();
		// }
		// else {
		// 	_channels[channelName]->setChannelTopic( user, newTopic );
		// }
		if (ircMessage.getParams().size() > 0){
				//check channel is exicst
				std::string channelname = ircMessage.getParams()[0];
				if (channelname[0] != '#') return;
				std::string cleanChannelName = channelname.substr(1);
				if (!isChannelExist(cleanChannelName)) {
					//:irc.localhost 403 alice #somechan :No such channel
					std::string reply = ":jarvis_server 403 " + user.getNickname() + " #" + cleanChannelName + " :No such channel" + "\r\n";
					user.sendMessage( reply );
					return;
				}
				if (ircMessage.getTrailing().empty()){
					// return topic
					std::string reply = ":jarvis_server 332 " + user.getNickname() + " #" + cleanChannelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n";
					user.sendMessage( reply );
					return ;
				}
				_channels[cleanChannelName]->setChannelTopic(user, ircMessage.getTrailing());
				std::string reply = ":jarvis_server 332 " + user.getNickname() + " #" + cleanChannelName + " :" + _channels[cleanChannelName]->getChannelTopic() + "\r\n";
				user.sendMessage( reply );

			}
	}
