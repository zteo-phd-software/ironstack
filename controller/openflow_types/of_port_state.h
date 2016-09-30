#ifndef __OPENFLOW_PORT_STATE
#define __OPENFLOW_PORT_STATE

#include <string>
#include "../../common/openflow.h"

// port status on a switch
// author : Z. Teo (zteo@cs.cornell.edu)
// updated: 3/18/14

class of_port_state {
public:

	// constructors
	of_port_state();
	of_port_state(uint32_t raw_value);

	void clear();

	// publicly accessible switches
	bool port_link_down;
	bool port_stp_listen;
	bool port_stp_learn;
	bool port_stp_forward;
	bool port_stp_block;
	bool port_stp_mask;

	void set(uint32_t raw_value);
	uint32_t get() const;

	std::string to_string() const;
};

#endif
