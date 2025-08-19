#include "Channel.hpp"
#include "User.hpp"
#include "Irc_server.hpp"

void	IrcServer::joinCmd( User &user, std::string channelName, std::string key ){
    // Check if channel name is valid
    if (channelName.empty() || channelName[0] != '#') {
        user.sendMessage(":jarvis_server 403 " + user.getNickname() + " " + channelName + " :Invalid channel name\r\n");
        return ;
    }

    // Remove # from channel name for internal storage
    std::string cleanChannelName = channelName.substr(1);

    // Create new channel if it doesn't exist
    if (!isChannelExist(cleanChannelName)) {
        _channels[cleanChannelName] = new Channel(cleanChannelName, user, key);
    }
    else {
        
    }

}