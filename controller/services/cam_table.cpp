#include "cam_table.h"
#include "../gui/output.h"

// default constructor
cam_table::cam_table() {
	max_capacity = 65536;
	timeout_sec = 14400;
}

// destructor
cam_table::~cam_table() {
}

// set up capacity
void cam_table::set_max_capacity(uint32_t max) {
	
	// enlarging is more convenient
	if (max > max_capacity) {
		max_capacity = max;

	// reduction in size needs to be accompanied by garbage collection
	} else {

		max_capacity = max;
		do_garbage_collection();
	}
}

// get max CAM table capacity
uint32_t cam_table::get_max_capacity() const {
	return max_capacity;
}

// get number of remaining entries in the cam table
uint32_t cam_table::get_available_capacity() const {
	return max_capacity <= cam_mappings.size() ? 0 : max_capacity - cam_mappings.size();
}

// setup a cam entry timeout
void cam_table::set_timeout(uint32_t timeout) {
	if (timeout >= timeout_sec) {
		timeout_sec = timeout;
	} else {
		timeout_sec = timeout;
		do_garbage_collection();
	}
}

// returns the cam entry timeout
uint32_t cam_table::get_timeout() const {
	return timeout_sec;
}

// clear the entire cam table
void cam_table::clear() {
	cam_mappings.clear();
}

// remove entries that are stale
void cam_table::do_garbage_collection() {

	int excess_entries = cam_mappings.size()-max_capacity;
	auto iterator = cam_mappings.begin();
	while (iterator != cam_mappings.end()) {
		if (!iterator->second.valid ||
			iterator->second.last_updated.get_time_elapsed_ms() >= (int)timeout_sec*1000) {
			iterator = cam_mappings.erase(iterator);
			--excess_entries;
		} else {
			++iterator;
		}
	}

	iterator = cam_mappings.begin();
	while(excess_entries > 0 && cam_mappings.size() > 0) {
		iterator = cam_mappings.erase(iterator);
		--excess_entries;
	}
}

// returns the port number for a mac address, or -1 if not found
int cam_table::lookup_port_for(const mac_address& dest) const {
	auto iterator = cam_mappings.find(dest);
	if (iterator == cam_mappings.end()) {
		return -1;
	} else {

		if (!iterator->second.valid) return -1;

		iterator->second.lookup_count++;
		iterator->second.last_lookup.reset();
		return iterator->second.phy_port;
	}
}

// inserts a cam entry
bool cam_table::insert(const mac_address& addr, uint16_t port) {
	bool result = false;
	auto iterator = cam_mappings.find(addr);
	if (iterator == cam_mappings.end()) {
		if (cam_mappings.size() >= max_capacity) {
			do_garbage_collection();
		}

		if (cam_mappings.size() >= max_capacity) {
			output::log(output::loglevel::ERROR, "cam_table::insert() ERROR failed to add entry dl_addr [%s] phy_port [%hu]. CAM table full!\n",
				addr.to_string().c_str(), port);
			result = false;
		} else {
			cam_mappings[addr] = move(cam_entry(addr, port));
			output::log(output::loglevel::INFO, "cam_table: inserted entry dl_addr [%s] --> phy_port [%hu].\n",
				addr.to_string().c_str(), port);
			result = true;
		}
	} else {
		iterator->second.valid = true;
		iterator->second.phy_port = port;
		iterator->second.update_count++;
		iterator->second.last_updated.reset();
		result = true;
	}

	return result;
}

// removes a cam entry
void cam_table::remove(const mac_address& dl_addr) {
	cam_mappings.erase(dl_addr);
}

// removes all cam entries on a port
void cam_table::remove(uint16_t port) {
	auto iterator = cam_mappings.begin();
	while(iterator != cam_mappings.end()) {
		if (iterator->second.phy_port == port) {
			iterator = cam_mappings.erase(iterator);
		} else {
			++iterator;
		}
	}
}

// retrieves a copy of the cam table
map<mac_address, cam_entry> cam_table::get_cam_table() const {
	return cam_mappings;
}
