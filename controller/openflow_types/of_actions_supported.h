#ifndef __OPENFLOW_SUPPORTED_ACTION_TYPES
#define __OPENFLOW_SUPPORTED_ACTION_TYPES

#include <string>
#include "../../common/openflow.h"

class of_actions_supported {
public:

	// default constructors
	of_actions_supported();
	of_actions_supported(uint32_t raw_value);

	void clear();

	// user-accessible actions
	bool output_to_switch_port;
	bool set_vlan_vid;
	bool set_vlan_pcp;
	bool strip_vlan_hdr;
	bool set_ethernet_src;
	bool set_ethernet_dest;
	bool set_ip_src;
	bool set_ip_dest;
	bool set_ip_type_of_service;
	bool set_tcpudp_src_port;
	bool set_tcpudp_dest_port;
	bool enqueue;
	bool vendor;

	// setter/getter methods to convert to/from raw values
	void set(uint32_t raw_value);
	uint32_t get() const;

	std::string to_string() const;
};

#endif
