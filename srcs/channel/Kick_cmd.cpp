#include "Irc_server.hpp"

//add the kick command
// the kick is not implementd on away to be copied here

void   IrcServer::kickCmd( User &user, Irc_message &ircMessage ){
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
        std::string userNick = ircMessage.getParams()[1];
        if (!_channels[cleanChannelName]->isUserInChannel(userNick)) {
            // 441 <user> <target> <channel> :They aren't on that channel
            std::string reply = ":jarvis_server 441 " + user.getNickname() + " " + userNick + " #" + cleanChannelName + " :They aren't on that channel" + "\r\n";
            user.sendMessage( reply );
            return;
        }
        _channels[cleanChannelName]->kickUser(user, userNick);
    }
}
