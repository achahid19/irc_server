#include "Irc_server.hpp"

void	IrcServer::topicCmd( User &user, std::string channelName, std::string newTopic)
	{
		//check if channel exists
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		//check if user in channel that will be done at the done inside the channel class
		if ( newTopic.empty() ){
			// viewTopic();
		}
		else {
			_channels[channelName]->setChannelTopic( user, newTopic );
		}
	}
