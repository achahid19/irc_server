#include "utils.hpp"

/**
 * to_string - convert an integer to a string.
 *
 * @param value: the integer value to convert.
 *
 * Return: the string representation of the integer.
 */
std::string to_string( int value ) {
	std::stringstream ss;

	ss << value;
	return ss.str();
}

/**
 * printMsg - print a message to the console with a timestamp.
 *
 * @param msg: the message to print.
 * @param logLevel: the log level (0 for no logs, 1 for info, 2 for debug).
 * @param color: the color code for the message.
 *
 * Return: void.
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
 * printErr - print an error message to the console with a timestamp.
 *
 * @param err: the error message to print.
 *
 * Return: void.
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

//CHA
int parsePositiveInteger(const std::string& s) {
    if (s.empty()) return 0;

    char* endptr = 0;
    errno = 0;

    long val = std::strtol(s.c_str(), &endptr, 10);

    // Check if entire string was parsed
    if (*endptr != '\0') return 0;

    // Check for range errors and positivity
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || val > INT_MAX || val < 1) {
        return 0;
    }

    return static_cast<int>(val);
}