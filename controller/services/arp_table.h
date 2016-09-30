#pragma once

#include <stdint.h>
#include <map>
#include <vector>
#include "../../common/ip_address.h"
#include "../../common/mac_address.h"
#include "../../common/timer.h"

class arp_entry {
public:
	arp_entry():valid(false),
		persistent(false),
		lookup_count(0),
		update_count(0) {		
		dl_address.clear();
		nw_address.clear();
		last_lookup.clear();
		last_updated.clear();
	};
	arp_entry(const mac_address& dl, const ip_address& nw):
		valid(true),
		persistent(false),
		dl_address(dl),
		nw_address(nw),
		lookup_count(0),
		update_count(0) {};

	// accessible fields
	bool             valid;
	bool             persistent;
	mac_address      dl_address;
	ip_address       nw_address;

	mutable	uint32_t lookup_count;
	mutable timer    last_lookup;

	uint32_t         update_count;
	timer            last_updated;
};

// defines the arp table class
class arp_table {
public:

	arp_table();
	~arp_table();

	// capacity setup
	void     set_max_capacity(uint32_t max);
	uint32_t get_max_capacity() const;
	uint32_t get_available_capacity() const;

	// entry timeout
	void     set_timeout(uint32_t timeout_sec);
	uint32_t get_timeout() const;

	// clears the arp table, perserving only persistent entries
	void     clear();
	void     clear_all(); // also removes persistent entries

	// remove stale entries
	void     do_garbage_collection();

	// returns mac address for a given ip address
	mac_address lookup_mac_for(const ip_address& dest);

	// returns ip address for a given mac
	ip_address lookup_ip_for(const mac_address& dest);

	// insert and delete
	bool     insert(const mac_address& dl_addr, const ip_address& nw_addr);
	bool     insert_persistent(const mac_address& dl_addr, const ip_address& nw_addr);
	void     remove(const mac_address& dl_addr);
	void     remove(const ip_address& nw_addr);

	// retrieves a copy of the arp table (expensive operation)
	// not const because this operation invalidates old entries
	vector<arp_entry> get_arp_table();

private:

	uint32_t max_capacity;
	uint32_t timeout_sec;
	uint32_t last_search_slot;
	
	map<mac_address, uint32_t> mac_to_idx_mapping;
	map<ip_address, uint32_t> ip_to_idx_mapping;
	vector<arp_entry> arp_mappings;

	// helper functions
	bool     is_entry_usable(arp_entry& entry);
	bool     get_free_index(uint32_t* index);

	// check if an existing mapping is correct
	bool     is_mapping_correct(const mac_address& dl_addr, const ip_address& nw_addr);

};
