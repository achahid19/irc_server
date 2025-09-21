#pragma once

#include <map>
#include <iostream>
#include <fstream>

class factBot {
private:
	std::map<std::string, std::string > facts;
			
	// helper methods
	void	loadFactsFromFile( const std::string &filename );

	// no instantiation, no copy
	factBot( void );
	factBot( const factBot& copy );
	factBot &operator=( const factBot& other );

public:
	factBot( const std::string &filename );
	~factBot( void );

	// methods
	void		addFact( const std::string& key, const std::string& fact );
	std::string	getFact( const std::string& key ) const;
	//void		listFacts( void ) const;
};
