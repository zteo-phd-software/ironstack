#pragma once
#include <stdint.h>
#include <string>
#include <set>
#include "../../common/mac_address.h"
#include "../../common/ip_address.h"
using namespace std;

// information about the port description
class switch_port_info {
public:

	uint16_t port_id;
	string   description;

	// generates a readable version of the information
	string to_string() const;
};

// information about each vlan
class vlan_info {
public:

	uint16_t      vlan_id;
	bool          is_openflow_vlan;
	string        vlan_name;

	set<uint16_t> tagged_ports;			// as per the OPENFLOW numbering, not manufacturer numbering
	set<uint16_t> untagged_ports;		// again as per the OPENFLOW numbering

	// generates a readable version of the information
	string to_string() const;
};


// information as gleaned from the csv object
class aux_switch_info {
public:

	void        clear();

	// directly accessible fields
	bool        valid;
	string      room;
	string      switch_name;
	string      switch_type;
	string      switch_location;
	ip_address  management_ip;
	ip_address  control_plane1_ip;
	ip_address  control_plane2_ip;
	ip_address  control_plane3_ip;
	mac_address hardware_address;
	mac_address datapath_id;
	ip_address  data_plane_ip;

	// generates a readable version of the information
	string to_string() const;

};
