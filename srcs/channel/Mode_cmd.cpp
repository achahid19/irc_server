#include "Irc_server.hpp"

void IrcServer::modeCmd( User &user, std::string channelName, std::string mode, std::string param ){
		//check if channel exists
		if (isChannelExist(channelName) == false){
			// 403 ERR_NOSUCHCHANNEL <channel> :No such channel
			return ;
		}
		if (mode == "+i"){
			_channels[channelName]->setInviteOnly(user);
		}
		else if (mode == "-i"){
			_channels[channelName]->removeInviteOnly(user);
		}
		else if (mode == "+t"){
			_channels[channelName]->setTopicOps(user);
		}
		else if (mode == "-t"){
			_channels[channelName]->removeTopicOps(user);

		}
		else if (mode == "+k"){
			_channels[channelName]->setKey(user, param);
		}
		else if (mode == "-k"){
			_channels[channelName]->removeKey(user);

		}
		else if (mode == "+l"){
			_channels[channelName]->setLimit(user, param);
		}
		else if (mode == "-l"){
			_channels[channelName]->removeLimit(user);

		}
		else if (mode == "+o"){
			_channels[channelName]->setOps(user, param);
		}
		else if (mode == "-o"){
			_channels[channelName]->removeOps(user, param);

		}
		else {}
	}
