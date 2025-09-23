#include "Irc_server.hpp"

std::string intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string modess( Channel *chan , User &user){

	std::stringstream info;
	std::string r = ":jarvis_server 324 " + user.getNickname() + " #" + chan->getChannelName() + " ";
	std::string modes;
    if (chan->isInviteOnly()) modes.append("i");
    if (chan->isTopicOps()) modes.append("t");
    if (chan->isLimitSet()) modes.append("l");
    if (chan->isKeyRequired()) modes.append("k");
	if (chan->isLimitSet()){
		modes.append(" MAX=");
		modes.append(intToString(chan->getChannelMaxUsers()));
	}
	if (chan->isKeyRequired()){modes.append(" "); modes.append(chan->getChannelKey());}

    (modes.empty() ? r += "no mode \r\n" : r += "+" + modes + "\r\n");

	return r;
}



void IrcServer::modeCmd( User &user, Irc_message &ircMessage ){


	if (ircMessage.getParams().size() >= 1){
			//check channel is exicst
			std::string channelname = ircMessage.getParams()[0];
			std::string cleanChannelName = channelname.substr(1);

			if (channelname[0] != '#') return;
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
			if (ircMessage.getParams().size() == 1){
				
				// std::cout << "[[[[[[[[   SERVER   ]]]]]]]]" << std::endl;
				user.sendMessage(modess( this->findChannelObj(cleanChannelName) , user));
				return ;

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

