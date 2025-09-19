#include "Irc_server.hpp"

void IrcServer::modeCmd( User &user, Irc_message &ircMessage ){
if (ircMessage.getParams().size() > 1){
			//check channel is exicst
			std::string channelname = ircMessage.getParams()[0];
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
			//check if the flages are only the allowd alphabitic ilkot   - +locgic start @params[1]
			std::string modeType = ircMessage.getParams()[1];
			if (modeType.size() > 1){
				char sign = modeType[0];
				for (size_t i = 1; i < modeType.size(); i++){
						if (modeType[i] == 'i' && sign == '+'){
						_channels[cleanChannelName]->setInviteOnly(user);
					}
					else if (modeType[i] == 'i' && sign == '-'){
						_channels[cleanChannelName]->removeInviteOnly(user);
					}
					else if (modeType[i] == 't' && sign == '+'){
						_channels[cleanChannelName]->setTopicOps(user);
					}
					else if (modeType[i] == 't' && sign == '-'){
						_channels[cleanChannelName]->removeTopicOps(user);
					}
					else if (modeType[i] == 'l' && sign == '+'){
						if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
							std::string limit = ircMessage.getParams()[2];
							_channels[cleanChannelName]->setLimit(user, limit);
						}
						else {
							//:server 461 <nick> MODE :Not enough parameters
							std::string reply = ":jarvis_server 461 " + user.getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
							user.sendMessage( reply );
							return ;
						}
					}
					else if (modeType[i] == 'l' && sign == '-'){
						_channels[cleanChannelName]->removeLimit(user);
					}
					else if (modeType[i] == 'k' && sign == '+'){
						if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
							std::string key = ircMessage.getParams()[2];
							_channels[cleanChannelName]->setKey(user, key);
							std::cout << "***************************\n";
						}
						else {
							//:server 461 <nick> MODE :Not enough parameters
							std::string reply = ":jarvis_server 461 " + user.getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
							user.sendMessage( reply );
							return ;
						}
					}
					else if (modeType[i] == 'k' && sign == '-'){
							_channels[cleanChannelName]->removeKey(user);
					}
					else if (modeType[i] == 'o' && sign == '+'){
						if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
							std::string newOps = ircMessage.getParams()[2];
							_channels[cleanChannelName]->setOps(user, newOps);
						}
						else {
							//:server 461 <nick> MODE :Not enough parameters
							std::string reply = ":jarvis_server 461 " + user.getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
							user.sendMessage( reply );
							return ;
						}
					}
					else if (modeType[i] == 'o' && sign == '-'){
						if (ircMessage.getParams().size() > 2 &&  !ircMessage.getParams()[2].empty()) {
							std::string newOps = ircMessage.getParams()[2];
							_channels[cleanChannelName]->removeOps(user, newOps);
						}
						else {
							//:server 461 <nick> MODE :Not enough parameters
							std::string reply = ":jarvis_server 461 " + user.getNickname() + " MODE " + " :Not enough parameters" + "\r\n";
							user.sendMessage( reply );
							return ;
						}
					}

				}
			}
		}
}
