#include "utils.hpp"

std::string to_string( int value ) {
	std::stringstream ss;

	ss << value;
	return ss.str();
}

/**
 * 
 */
void	printMsg(
	std::string const& msg,
	int logLevel,
	const char *color
) {
	logLevel && \
	std::cout << color << Time::getCurrentTime() << " " \
	<< msg << COLOR_RESET <<std::endl;
}

/**
 * 
 */
void	printErr(
	std::string const& err
) {
	ERROR_LOGS && \
	std::cerr << COLOR_RED << Time::getCurrentTime() << " " \
	<< err << COLOR_RESET <<std::endl;
}

/**
 * ft_trim_spaces - trim leading and trailing spaces from a string.
 * @param str: the string to trim.
 * 
 * Return: trimmed string.
 */
std::string	ft_trim_spaces( const std::string &str ) {
	size_t start = str.find_first_not_of(" \t\n\r\f\v");
	if (start == std::string::npos) {
		return ""; // string is all spaces
	}
	size_t end = str.find_last_not_of(" \t\n\r\f\v");
	return str.substr(start, end - start + 1);
}
