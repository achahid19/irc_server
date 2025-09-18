#include "Irc_server.hpp"

void	IrcServer::privCmd( User &user, std::string channelName, std::string message ){
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		_channels[channelName]->pingUser( user, message );
	}
