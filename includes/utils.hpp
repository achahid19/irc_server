#pragma once

#include <iostream>
#include <sstream>

enum {
	INFO_LOGS = true,
	ERROR_LOGS = true,
	DEBUG_LOGS = false,
	WARNING_LOGS = false,
};

std::string to_string( int value );

// time class with static methods
class Time {
public:
	static std::string getCurrentTime() {
		time_t now = time(0);
		struct tm tstruct;
		char buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "[ %Y-%m-%d %X ]", &tstruct);
		return std::string(buf);
	}
	static std::string getCurrentDate() {
		time_t now = time(0);
		struct tm tstruct;
		char buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "[ %Y-%m-%d ]", &tstruct);
		return std::string(buf);
	}
	static std::string getCurrentTimeWithMs() {
		time_t now = time(0);
		struct tm tstruct;
		char buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "[ %Y-%m-%d %X ]", &tstruct);
		std::stringstream ss;
		ss << buf << "." << (now % 1000); // append milliseconds
		return ss.str();
	}
};
