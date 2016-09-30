#include "arp_table.h"
#include "../gui/output.h"

// default constructor
arp_table::arp_table() {
	max_capacity = 16384;
	timeout_sec = 14400;
	last_search_slot = 0;

//	arp_entry entry;
	arp_mappings.reserve(max_capacity);
	for (uint32_t counter = 0; counter < max_capacity; ++counter) {
		arp_mappings.emplace_back(arp_entry());
//		arp_mappings.push_back(entry);
	}
}

// destructor
arp_table::~arp_table() {
}

// sets up the table capacity
void arp_table::set_max_capacity(uint32_t max) {
	if (max < max_capacity) {
		max_capacity = max;
		do_garbage_collection();
	} else {
		uint32_t difference = max - max_capacity;
		max_capacity = max;
		arp_mappings.reserve(max_capacity);
		for (uint32_t counter = 0; counter < difference; ++counter) {
			arp_mappings.push_back(arp_entry());
		}
	}
}

// returns the max table capacity
uint32_t arp_table::get_max_capacity() const {
	return max_capacity;
}

// checks the number of table entries left
uint32_t arp_table::get_available_capacity() const {
	if (max_capacity <= mac_to_idx_mapping.size()) {
		return 0;
	} else {
		return max_capacity - mac_to_idx_mapping.size();
	}
}

// sets the arp entry timeout
void arp_table::set_timeout(uint32_t timeout) {
	if (timeout_sec >= timeout_sec) {
		timeout_sec = timeout;
	} else {
		timeout_sec = timeout;
		do_garbage_collection();
	}

}

// gets the arp entry timeout
uint32_t arp_table::get_timeout() const {
	return timeout_sec;
}

// clears all entries in the arp table, perserving only the
// persistent ones
void arp_table::clear() {

	auto iterator = mac_to_idx_mapping.begin();
	while (iterator != mac_to_idx_mapping.end()) {
		if (!arp_mappings[iterator->second].persistent) {
			ip_to_idx_mapping.erase(arp_mappings[iterator->second].nw_address);
			iterator = mac_to_idx_mapping.erase(iterator);
		} else {
			++iterator;
		}
	}
}

// clears all entries in the arp table, including persistent ones
void arp_table::clear_all() {
	mac_to_idx_mapping.clear();
	ip_to_idx_mapping.clear();
	
	uint32_t entries = arp_mappings.size();
	for (uint32_t counter = 0; counter < entries; ++counter) {
		arp_mappings[counter].valid = false;
	}
}

// remove stale entries
// TODO: compact table
void arp_table::do_garbage_collection() {
	
	auto iterator = mac_to_idx_mapping.begin();
	while (iterator != mac_to_idx_mapping.end()) {
		arp_entry& entry = arp_mappings[iterator->second];
		if (entry.last_updated.get_time_elapsed_ms() >= (int)timeout_sec*1000) {
			entry.valid = false;
			ip_to_idx_mapping.erase(entry.nw_address);
			iterator = mac_to_idx_mapping.erase(iterator);
		} else {
			++iterator;
		}
	}
}

// given an IP address, look up its mac address
mac_address arp_table::lookup_mac_for(const ip_address& dest) {

	auto iterator = ip_to_idx_mapping.find(dest);
	if (iterator == ip_to_idx_mapping.end()) {
		output::log(output::loglevel::VERBOSE, "arp_table::lookup_mac() -- no entry for %s.\n", dest.to_string().c_str());
		return mac_address();
	}

	// check validity, timeout and staleness
	arp_entry& entry = arp_mappings[iterator->second];
	if (!is_entry_usable(entry)) {
		return mac_address();
	}

	// update stats
	entry.lookup_count++;
	entry.last_lookup.reset();

	return entry.dl_address;
}

// given a MAC address, look up its IP address
ip_address arp_table::lookup_ip_for(const mac_address& dest) {

	auto iterator = mac_to_idx_mapping.find(dest);
	if (iterator == mac_to_idx_mapping.end()) {
		return ip_address();
	}

	// check validity, timeout and staleness
	arp_entry& entry = arp_mappings[iterator->second];
	if (!is_entry_usable(entry)) {
		return ip_address();
	}

	// update stats
	entry.lookup_count++;
	entry.last_lookup.reset();

	return entry.nw_address;
}

// inserts an nonpersistent entry into the arp table
bool arp_table::insert(const mac_address& dl_addr, const ip_address& nw_addr) {
	
	if (dl_addr.is_nil() || nw_addr.is_nil()) {
		output::log(output::loglevel::WARNING, "arp_table::insert() -- cannot insert [%s]/[%s] (invalid address)\n",
			dl_addr.to_string().c_str(),
			nw_addr.to_string().c_str());
		return false;
	}

	// TODO -- this fixes up the gateway problem. don't do rigorous mapping correctness checks for
	// gateway addresses
	if (nw_addr[3] == 1) {
//		auto iterator1 = mac_to_idx_mapping.find(dl_addr);
		auto iterator2 = ip_to_idx_mapping.find(nw_addr);
		if (iterator2 != ip_to_idx_mapping.end()) return true;

	} else {
		// non gateway addresses.
		// remove the old entries if they exist
		if (is_mapping_correct(dl_addr, nw_addr)) {
			return true;
		}
	}

	// mapping isn't correct or doesn't exist, so update the table
	// TODO -- this fixes up the gateway problem. only remove mappings for non-gateway
	// IP addresses. otherwise, gateways can have multiple mappings (eg 128.253.141.1 and 10.253.141.1)
	if (nw_addr[3] != 1) {
		remove(dl_addr);
		remove(nw_addr);
	}

	// add in the new entry
	uint32_t index;
	if (!get_free_index(&index)) {
		output::log(output::loglevel::CRITICAL, "arp_table::insert() -- cannot find free table slot.\n");
		return false;
	}

	mac_to_idx_mapping[dl_addr] = index;
	ip_to_idx_mapping[nw_addr] = index;
	arp_mappings[index] = arp_entry(dl_addr, nw_addr);
	output::log(output::loglevel::INFO, "arp_table::insert() -- added [%s] <--> [%s] at slot %u.\n",
		nw_addr.to_string().c_str(),
		dl_addr.to_string().c_str(),
		index);

	return true;
}

// inserts a persistent entry into the arp table
bool arp_table::insert_persistent(const mac_address& dl_addr, const ip_address& nw_addr) {
	if (dl_addr.is_nil() || dl_addr.is_broadcast() || nw_addr.is_nil() || nw_addr.is_broadcast()) {
		output::log(output::loglevel::WARNING, "arp_table::insert_persistent() -- cannot insert [%s]/[%s] (invalid address.\n",
			dl_addr.to_string().c_str(),
			nw_addr.to_string().c_str());

		return false;
	}

	// remove the old entries if they exist
	remove(dl_addr);
	remove(nw_addr);

	// add in the new entry
	uint32_t index;
	if (!get_free_index(&index)) {
		output::log(output::loglevel::CRITICAL, "arp_table::insert() -- cannot find free table slot.\n");
		return false;
	}

	mac_to_idx_mapping[dl_addr] = index;
	ip_to_idx_mapping[nw_addr] = index;
	arp_mappings[index] = arp_entry(dl_addr, nw_addr);
	arp_mappings[index].persistent = true;

	output::log(output::loglevel::INFO, "arp_table: inserted [%s] <--> [%s] [PERSISTENT]\n",
		nw_addr.to_string().c_str(),
		dl_addr.to_string().c_str());

	return true;
}

// removes an entry from the table
void arp_table::remove(const mac_address& dl_addr) {
	auto iterator = mac_to_idx_mapping.find(dl_addr);
	if (iterator != mac_to_idx_mapping.end()) {
		arp_entry& entry = arp_mappings[iterator->second];
		ip_to_idx_mapping.erase(entry.nw_address);
		mac_to_idx_mapping.erase(iterator);
		entry.valid = false;
	}
}

// removes an entry from the table
void arp_table::remove(const ip_address& nw_addr) {
	auto iterator = ip_to_idx_mapping.find(nw_addr);
	if (iterator != ip_to_idx_mapping.end()) {
		arp_entry& entry = arp_mappings[iterator->second];
		mac_to_idx_mapping.erase(entry.dl_address);
		ip_to_idx_mapping.erase(iterator);
		entry.valid = false;
	}
}

// retrieves the arp table
vector<arp_entry> arp_table::get_arp_table() {

	vector<arp_entry> result;
	result.reserve(mac_to_idx_mapping.size());
	for (const auto& it : mac_to_idx_mapping) {
		arp_entry& entry = arp_mappings[it.second];
		if (is_entry_usable(entry)) {
			result.push_back(entry);
		}
	}

	return result;
}

// invalidates a certain entry if it is stale and is non persistent
// returns true if the entry is still valid or persistent
bool arp_table::is_entry_usable(arp_entry& entry) {

	if (!entry.valid) {
		return false;
	} else if (entry.persistent) {
		return true;
	} else if (entry.last_updated.get_time_elapsed_ms() >= (int)timeout_sec * 1000) {
		entry.valid = false;
		mac_to_idx_mapping.erase(entry.dl_address);
		ip_to_idx_mapping.erase(entry.nw_address);
		return false;
	} else {
		return true;
	}
}

// finds a free index which to insert an entry
bool arp_table::get_free_index(uint32_t* result) {

	uint32_t index = (last_search_slot+1) % max_capacity;
	while (arp_mappings[index].valid && index != max_capacity) {
		index = (index+1)% max_capacity;
	}

	if (arp_mappings[index].valid) {
		return false;
	}

	*result = index;
	last_search_slot = index;
	return true;
}

// checks if a mapping is correct
bool arp_table::is_mapping_correct(const mac_address& dl_addr, const ip_address& nw_addr) {

	auto iterator1 = mac_to_idx_mapping.find(dl_addr);
	auto iterator2 = ip_to_idx_mapping.find(nw_addr);

	if (iterator1 == mac_to_idx_mapping.end() || iterator2 == ip_to_idx_mapping.end()) {
		bool mac_invalid = (iterator1 == mac_to_idx_mapping.end());
		bool ip_invalid = (iterator2 == ip_to_idx_mapping.end());

		if (!(mac_invalid && ip_invalid)) {
			output::log(output::loglevel::WARNING, "arp_table::is_mapping_correct() invalid mapping! mac [%s]:[slot %d] <--> ip [%s]:[slot %d]\n",
				dl_addr.to_string().c_str(),
				mac_invalid ? -1 : iterator1->second,
				nw_addr.to_string().c_str(),
				ip_invalid ? -1 : iterator2->second);
		}
		return false;
	}

	if (iterator1->second != iterator2->second) {
		output::log(output::loglevel::WARNING, "arp_table::is_mapping_correct() -- mapping mismatch!\n"
			"proposed mapping [%s] <--> [%s]\n"
			"conflicts with   [%s] <--> [%s]\n"
			"                 [%s] <--> [%s]\n"
			"this could be a benign  address change, or a malicious attempt at address spoofing.\n",
			dl_addr.to_string().c_str(),
			nw_addr.to_string().c_str(),
			arp_mappings[iterator1->second].dl_address.to_string().c_str(),
			arp_mappings[iterator1->second].nw_address.to_string().c_str(),
			arp_mappings[iterator2->second].dl_address.to_string().c_str(),
			arp_mappings[iterator2->second].nw_address.to_string().c_str());
		return false;
	} else {
		return true;
	}
}
