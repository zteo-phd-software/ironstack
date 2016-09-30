#ifndef __OPENFLOW_PORT_CONFIGURATION
#define __OPENFLOW_PORT_CONFIGURATION

#include <string>
#include "../../common/openflow.h"

class openflow_port_config {
public:

	// constructors
	openflow_port_config();
	openflow_port_config(uint32_t raw_value);

	void clear();

	// user-accessible switches
	bool port_down;											// port is administratively down
	bool port_stp_disabled;							// disable spanning tree on port
	bool port_drop_all_pkts_except_stp;	// drop all packets except spanning
																			// tree packets
	bool port_drop_stp_pkts;						// drop only stp packets
	bool port_exclude_from_flooding;		// exclude this port when flooding
	bool port_drop_all_pkts;						// drop all packets forwarded to this
																			// port
	bool port_pkt_in_events_disabled;		// disable packet-in events to the
																			// controller for this port

	// setter, getter methods
	void set(uint32_t raw_value);
	uint32_t get() const;

	// generates a readable version of this object
	std::string to_string() const;
};

#endif
