#pragma once

#include <iostream>
#include <sstream>
#include <cstdlib>  // for strtol
#include <cerrno>   // for errno
#include <climits>  // for INT_MAX


// define colors
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_GRAY "\033[90m"


enum logLevels {
	INFO_LOGS = false,
	ERROR_LOGS = false,
	DEBUG_LOGS = false,
	WARNING_LOGS = false,
};

std::string to_string( int value );
void	printMsg(
	std::string const& msg,
	int logLevel = true,
	const char *color = "\033[0m"
);

void	printErr( std::string const& err );

// time class with static methods
class Time {
public:
	static std::string getCurrentTime() {
		char buf[80];
		time_t now = time(NULL);
		strftime(buf, sizeof(buf), "[ %X ]", localtime(&now));
		return buf;
	}
};

std::string	ft_trim_spaces( const std::string &str );

//CHA
int parsePositiveInteger(const std::string& s);
