#ifndef __OPENFLOW_SWITCH_CAPABILITIES
#define __OPENFLOW_SWITCH_CAPABILITIES

#include <string>
#include "../../common/openflow.h"

// switch capabilities
// author: Z. Teo (zteo@cs.cornell.edu)

class of_switch_capabilities {
public:

	// constructors
	of_switch_capabilities();
	of_switch_capabilities(uint32_t raw_value);
	void clear();

	// supported modes
	bool switch_flow_stats_supported;
	bool switch_table_stats_supported;
	bool switch_port_stats_supported;
	bool switch_stp_supported;
	bool switch_ip_reassembly_supported;
	bool switch_queue_stats_supported;
	bool switch_arp_match_ip_supported;

	// take in raw valuea and modify state
	void set(uint32_t raw_value);
	uint32_t get() const;

	std::string to_string() const;
};

#endif

