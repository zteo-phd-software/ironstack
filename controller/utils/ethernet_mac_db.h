#pragma once
#include <map>
#include <tuple>
#include <vector>
#include "../../common/autobuf.h"
#include "../../common/mac_address.h"
using namespace std;

// ethernet mac address database lookup utility
class ethernet_mac_db {
public:

	// constructor and destructor
	ethernet_mac_db();
	~ethernet_mac_db();

	// load the file
	bool     load_file(const string& filename);
	uint32_t get_num_entries() const;
	
	// lookup function
	string lookup_name_for_mac(const mac_address& addr) const;

	// generates a readable version of the table
	string to_string() const;

private:

	vector<tuple<string, uint8_t, string>> high_specificity_table;	// linear scan, for masked or fully specified addresses
	map<string, string>                    low_specificity_table;   // mapped table for a /24 table

};
