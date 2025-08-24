#include "Irc_server.hpp"

void	IrcServer::partCmd( User &user, std::string channelName, std::string reason ){
    if (isChannelExist(channelName) == false){
        // 403 ERR_NOSUCHCHANNEL <channel> :No such channel
        user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :No such channel\r\n");
        return ;
    }
    
    // Remove # from channel name for internal storage
    std::string cleanChannelName = channelName;
    if (channelName[0] == '#') {
        cleanChannelName = channelName.substr(1);
    }
    
    // Check if user is in the channel
    if (!_channels[cleanChannelName]->isUserInChannel(user.getNickname())) {
        user.sendMessage(":jarvis_server 442 " + user.getNickname() + " " + channelName + " :You're not on that channel\r\n");
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