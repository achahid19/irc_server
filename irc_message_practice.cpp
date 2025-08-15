#include "includes/Irc_message.hpp"
#include <iostream>
#include <string>

// Practice function to test Irc_message parsing
void testMessage(const std::string& rawMessage) {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "Testing: " << rawMessage << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    // Create Irc_message instance
    Irc_message ircMessage(rawMessage);
    
    // Parse the message
    ircMessage.parseMessage();
    
    // Display results
    std::cout << "Command: '" << ircMessage.getCommand() << "'" << std::endl;
    
    std::cout << "Parameters (" << ircMessage.getParams().size() << "): ";
    for (size_t i = 0; i < ircMessage.getParams().size(); i++) {
        std::cout << "[" << i << "]='" << ircMessage.getParams()[i] << "' ";
    }
    std::cout << std::endl;
    
    std::cout << "Trailing: '" << ircMessage.getTrailing() << "'" << std::endl;
    std::cout << "Has Prefix: " << (ircMessage.hasPrefix() ? "Yes" : "No") << std::endl;
    
    if (ircMessage.hasPrefix()) {
        std::cout << "Prefix: '" << ircMessage.getPrefix() << "'" << std::endl;
    }
}

// Practice function to simulate command handling
void handleCommand(const std::string& rawMessage) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "Handling Command: " << rawMessage << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    Irc_message ircMessage(rawMessage);
    ircMessage.parseMessage();
    
    std::string command = ircMessage.getCommand();
    
    if (command == "JOIN") {
        std::cout << "🎯 JOIN Command Detected!" << std::endl;
        if (ircMessage.getParams().size() > 0) {
            std::string channelName = ircMessage.getParams()[0];
            std::string key = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";
            std::cout << "   Channel: " << channelName << std::endl;
            std::cout << "   Key: " << (key.empty() ? "(none)" : key) << std::endl;
        } else {
            std::cout << "   ❌ Error: No channel specified" << std::endl;
        }
    }
    else if (command == "PART") {
        std::cout << "🚪 PART Command Detected!" << std::endl;
        if (ircMessage.getParams().size() > 0) {
            std::string channelName = ircMessage.getParams()[0];
            std::string reason = ircMessage.getTrailing();
            std::cout << "   Channel: " << channelName << std::endl;
            std::cout << "   Reason: " << (reason.empty() ? "(none)" : reason) << std::endl;
        } else {
            std::cout << "   ❌ Error: No channel specified" << std::endl;
        }
    }
    else if (command == "PRIVMSG") {
        std::cout << "💬 PRIVMSG Command Detected!" << std::endl;
        if (ircMessage.getParams().size() > 0) {
            std::string target = ircMessage.getParams()[0];
            std::string message = ircMessage.getTrailing();
            std::cout << "   Target: " << target << std::endl;
            std::cout << "   Message: " << message << std::endl;
        } else {
            std::cout << "   ❌ Error: No target specified" << std::endl;
        }
    }
    else if (command == "TOPIC") {
        std::cout << "📝 TOPIC Command Detected!" << std::endl;
        if (ircMessage.getParams().size() > 0) {
            std::string channelName = ircMessage.getParams()[0];
            std::string newTopic = ircMessage.getTrailing();
            std::cout << "   Channel: " << channelName << std::endl;
            std::cout << "   New Topic: " << (newTopic.empty() ? "(view current)" : newTopic) << std::endl;
        } else {
            std::cout << "   ❌ Error: No channel specified" << std::endl;
        }
    }
    else if (command == "MODE") {
        std::cout << "⚙️ MODE Command Detected!" << std::endl;
        if (ircMessage.getParams().size() > 0) {
            std::string channelName = ircMessage.getParams()[0];
            std::string mode = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";
            std::string param = (ircMessage.getParams().size() > 2) ? ircMessage.getParams()[2] : "";
            std::cout << "   Channel: " << channelName << std::endl;
            std::cout << "   Mode: " << (mode.empty() ? "(view current)" : mode) << std::endl;
            std::cout << "   Parameter: " << (param.empty() ? "(none)" : param) << std::endl;
        } else {
            std::cout << "   ❌ Error: No channel specified" << std::endl;
        }
    }
    else if (command == "KICK") {
        std::cout << "👢 KICK Command Detected!" << std::endl;
        if (ircMessage.getParams().size() > 0) {
            std::string channelName = ircMessage.getParams()[0];
            std::string targetUser = (ircMessage.getParams().size() > 1) ? ircMessage.getParams()[1] : "";
            std::string reason = ircMessage.getTrailing();
            std::cout << "   Channel: " << channelName << std::endl;
            std::cout << "   Target User: " << (targetUser.empty() ? "(none)" : targetUser) << std::endl;
            std::cout << "   Reason: " << (reason.empty() ? "(none)" : reason) << std::endl;
        } else {
            std::cout << "   ❌ Error: No channel specified" << std::endl;
        }
    }
    else {
        std::cout << "❓ Unknown Command: " << command << std::endl;
    }
}

int main() {
    std::cout << "🎯 IRC_MESSAGE PRACTICE SESSION" << std::endl;
    std::cout << "================================" << std::endl;
    
    // ===== PRACTICE 1: Basic Message Parsing =====
    std::cout << "\n📚 PRACTICE 1: Basic Message Parsing" << std::endl;
    std::cout << "Learn how different message formats are parsed" << std::endl;
    
    testMessage("JOIN #test");
    testMessage("PART #test :Leaving channel");
    testMessage("PRIVMSG #test :Hello everyone!");
    testMessage("MODE #test +i");
    testMessage("TOPIC #test :New channel topic");
    testMessage("KICK #test baduser :Breaking rules");
    
    // ===== PRACTICE 2: Command Handling =====
    std::cout << "\n🎮 PRACTICE 2: Command Handling" << std::endl;
    std::cout << "Practice handling different commands like you'll do in your server" << std::endl;
    
    handleCommand("JOIN #test");
    handleCommand("JOIN #secret mypassword");
    handleCommand("PART #test :Goodbye everyone!");
    handleCommand("PRIVMSG #test :How is everyone doing?");
    handleCommand("TOPIC #test :Welcome to our channel!");
    handleCommand("MODE #test +o alice");
    handleCommand("KICK #test bob :Being disruptive");
    
    // ===== PRACTICE 3: Edge Cases =====
    std::cout << "\n⚠️ PRACTICE 3: Edge Cases" << std::endl;
    std::cout << "Test how the parser handles unusual inputs" << std::endl;
    
    testMessage("JOIN");  // No parameters
    testMessage("");      // Empty message
    testMessage("   JOIN   #test   ");  // Extra spaces
    testMessage("JOIN #test :");  // Empty trailing
    
    // ===== PRACTICE 4: Your Turn! =====
    std::cout << "\n🚀 PRACTICE 4: Your Turn!" << std::endl;
    std::cout << "Try creating your own messages and see how they're parsed" << std::endl;
    
    // Add your own test messages here:
    // testMessage("YOUR_MESSAGE_HERE");
    // handleCommand("YOUR_COMMAND_HERE");
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "🎯 PRACTICE SESSION COMPLETE!" << std::endl;
    std::cout << "You're now ready to implement channel commands!" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    return 0;
} 