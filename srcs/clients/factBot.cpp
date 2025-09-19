#include "factBot.hpp"
#include "utils.hpp"

/**
 * factBot - factBot constructor
 *
 * This method will the factBot and load facts from a given file into the facts map.
 *
 * @param filename: the name of the file containing facts in key-fact pairs.
 *
 * Return: void.
 */
factBot::factBot( const std::string &filename ) {
	this->loadFactsFromFile(filename);
}

/**
 * ~factBot - factBot destructor
 *
 * This method will be called when the factBot object is destroyed.
 *
 * Return: void.
 */
factBot::~factBot( void ) {};

/**
 * loadFactsFromFile - factBot private method
 *
 * This method will load facts from a given file into the facts map.
 * Each line in the file should contain a key and a fact separated by a colon.
 *
 * @param filename: the name of the file containing facts in key-fact pairs.
 *
 * Return: void.
 */
void	factBot::loadFactsFromFile( const std::string &filename ) {
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		printErr("Could not open facts file: " + filename);
		return;
	}
	std::string line;
	while (std::getline(file, line)) {
		size_t delimiterPos = line.find(':');
		if (delimiterPos != std::string::npos) {
			std::string key = ft_trim_spaces(line.substr(0, delimiterPos));
			std::string fact = ft_trim_spaces(line.substr(delimiterPos + 1));
			if (!key.empty() && !fact.empty()) {
				facts[key] = fact;
			}
		}
	}
	// print facts on server
	for (std::map<std::string, std::string>::iterator it = facts.begin(); it != facts.end(); ++it) {
		printMsg("Loaded fact - Key: " + it->first + ": " + it->second, DEBUG_LOGS, COLOR_YELLOW);
	}
	file.close();
}

/**
 * addFact - factBot public method
 *
 * This method will add a new fact to the facts map.
 *
 * @param key: the key for the fact.
 * @param fact: the fact associated with the key.
 *
 * Return: void.
 */
void	factBot::addFact( const std::string& key, const std::string& fact ) {
	if (key.empty() || fact.empty()) {
		printErr("Key or fact cannot be empty.");
		return;
	}
	facts[key] = fact;
	// fact also must be to the file
	std::ofstream file("facts.txt", std::ios::app);
	if (!file.is_open()) {
		printErr("Could not open facts file for appending.");
		return;
	}
	file << key << ": " << fact << std::endl;
	file.close();
}

/**
 * getFact - factBot public method
 *
 * This method will retrieve a fact from the facts map based on the given key.
 *
 * @param key: the key for the fact to retrieve.
 *
 * Return: std::string - the fact associated with the key, or an error message if not found.
 */
std::string	factBot::getFact( const std::string& key ) const {
	std::map<std::string, std::string>::const_iterator it = facts.find(key);
	if (it != facts.end()) {
		return it->second;
	} else {
		return "Fact not found for key: " + key;
	}
}
