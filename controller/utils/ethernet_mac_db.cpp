#include "ethernet_mac_db.h"
#include <unistd.h>
#include "../gui/output.h"

// constructor and destructor
ethernet_mac_db::ethernet_mac_db() {}
ethernet_mac_db::~ethernet_mac_db() {}

// load the file
bool ethernet_mac_db::load_file(const string& filename) {
	autobuf data;
	if (!data.load(filename)) return false;

	char buf[512];
	uint32_t file_size = data.size();
	for (uint32_t counter = 0; counter < file_size;) {

		// read each line into the buf
		uint32_t line_len = 0;
		for (uint32_t counter2 = counter; counter2-counter < sizeof(buf); ++counter2) {
			counter++;
			if (data[counter2] == '\n' || data[counter2] == '\r') {
				buf[line_len] = 0;
				break;
			} else {
				buf[line_len++] = data[counter2];
			}
		}

		// ignore comments and empty lines
		if (buf[0] == '#' || buf[0] == 0) continue;

		// handle low specificity (3 hex number prefixes only)
		if (line_len > 9 && buf[2] == ':' && buf[5] == ':' && buf[8] == '\t') {
			string prefix;
			prefix.assign(buf, 8);

			uint32_t result_size = 0;
			for (uint32_t counter2 = 9; counter2 < line_len; ++counter2) {
				if (buf[counter2] == '\t' || buf[counter2] == 0 || buf[counter2] == ' ') {
					break;
				} else {
					++result_size;
				}
			}

			string result;
			result.assign(buf, 9, result_size);
			low_specificity_table[prefix] = result;

		// handle high specificity
		} else {

			// TODO -- we ignore high specificity tables for now
			/*
			// check for masking and reform the mac address
			uint32_t mask = 64;
			string address;
			string result;
			
			for (uint32_t counter2 = 0; counter2 < line_len; ++counter2) {

				// handle addresses with mask
				if (buf[counter2] == '/') {
					address = string(buf, counter2);

					if (sscanf(buf+counter2+1, "%u", &mask) != 1) {
						return false;
					}
					for (uint32_t counter3 = counter2; counter3 < line_len; ++counter3) {
						if (buf[counter3] == '\t' || buf[counter3] == ' ') {

//							result = string(buf+counter3+1);
							uint32_t string_len = 0;
							for (uint32_t counter4 = counter3; counter4 < line_len; ++counter4) {
								if (buf[counter4] == '\t' || buf[counter4] == ' ') break;
								++string_len;
							}

							result = string(buf+counter3+1, string_len);
							break;
						}
					}
					break;

				// handle addresses without mask
				} else if (buf[counter2] == '\t' || buf[counter2] == ' ') {
					address = string(buf, counter2);
					result = string(buf+counter2+1);
					break;
				}
			}

			// insert entry
			if (!address.empty() && !result.empty()) {
				high_specificity_table.emplace_back(make_tuple(address, mask, result));
				output::printf("[%s] --> [%s] %u\n", address.c_str(), result.c_str(), mask);
			}
			*/
		}
	}
	
	return true;
}

// gets the number of entries in the tables
uint32_t ethernet_mac_db::get_num_entries() const {
	return high_specificity_table.size() + low_specificity_table.size();
}

// lookup function
string ethernet_mac_db::lookup_name_for_mac(const mac_address& addr) const {

	// generate address in uppercase string form
	string address = addr.to_string();
	uint32_t size = address.size();
	for (uint32_t counter = 0; counter < size; ++counter) {
		address[counter] = toupper(address[counter]);
	}

	// search low specificity table first (this is a performance hack, tables
	// should be searched from high to low but most addresses will be low
	// specificity).
	auto low_address = address.substr(0, 8);
	auto iterator = low_specificity_table.find(low_address);
	if (iterator != low_specificity_table.end()) {
		return iterator->second;
	}

	// search high specificity table
	for (const auto& entry : high_specificity_table) {
		string prefix;
		uint32_t mask;
		string result;

		tie(prefix, mask, result) = entry;
		if (mask == 0 || mask % 8 != 0) continue;		// TODO -- ignore non 8bit aligned masks for now

		uint32_t bytes = mask / 8;
		string address_to_check = address.substr(0, bytes*2 + (bytes-1));
		string prefix_to_check = prefix.substr(0, bytes*2 + (bytes-1));

		if (address_to_check == prefix_to_check) return result;
	}

	return string();
}

// generates a readable version of the table
string ethernet_mac_db::to_string() const {

	string result;
	for (const auto& entry : high_specificity_table) {
		string prefix;
		uint32_t mask;
		string suffix;

		tie(prefix, mask, suffix) = entry;
		char buf[32];
		sprintf(buf, "/%u: ", mask);
		result += prefix + string(buf) + suffix + string("\n");
	}

	for (const auto& entry : low_specificity_table) {
		string prefix = entry.first;
		string suffix = entry.second;

		result += prefix + string(": ") + suffix + string("\n");
	}

	return result;
}
