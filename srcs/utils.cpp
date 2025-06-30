#include "../includes/utils.hpp"

std::string to_string( int value ) {
	std::stringstream ss;

	ss << value;
	return ss.str();
}
