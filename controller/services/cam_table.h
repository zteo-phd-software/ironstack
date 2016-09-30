#pragma once

#include <map>
#include <stdint.h>
#include "../../common/mac_address.h"
#include "../../common/timer.h"
using namespace std;

// defines CAM entries and tables for use with the CAM service
// vlan separated.
//
// revision 1 (2/22/15)
// by Z. Teo

class cam_entry {
public:

	// default constructor
	cam_entry():
		valid(false),
		phy_port(0),
		lookup_count(0),
		update_count(0) {}

	// shortcut constructor
	cam_entry(const mac_address& addr, uint16_t port):
		valid(true),
		dl_address(addr),
		phy_port(port),
		lookup_count(0),
		update_count(0) { last_updated.start(); }

	// accessible fields
	bool             valid;
	mac_address      dl_address;
	uint16_t         phy_port;

	mutable uint32_t lookup_count;
	mutable timer    last_lookup;

	uint32_t         update_count;
	timer            last_updated;
};

// an aggregation of cam entries
class cam_table {
public:

	// default constructor
	cam_table();

	// destructor
	~cam_table();

	// capacity setup
	void     set_max_capacity(uint32_t max);
	uint32_t get_max_capacity() const;
	uint32_t get_available_capacity() const;

	// entry timeout
	void     set_timeout(uint32_t timeout_sec);
	uint32_t get_timeout() const;

	// clear the cam table (does not affect timeout or max capacity)
	void     clear();

	// remove entries that are stale
	void     do_garbage_collection();

	// returns uint16_t port or -1 if no port found
	int      lookup_port_for(const mac_address& dest) const;
	
	// insert and delete
	bool     insert(const mac_address& dl_addr, uint16_t port);
	void     remove(const mac_address& dl_addr);
	void     remove(uint16_t port);
	
	// retrieves a copy of the cam table. very expensive operation!
	map<mac_address, cam_entry> get_cam_table() const;

private:

	uint32_t                    max_capacity;
	uint32_t                    timeout_sec;
	map<mac_address, cam_entry> cam_mappings;
};


