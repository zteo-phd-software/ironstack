#ifndef __OPENFLOW_PORT_FEATURES
#define __OPENFLOW_PORT_FEATURES

#include <string>
#include "../../common/openflow.h"

// port/link capabilities
// author: Z. Teo (zteo@cs.cornell.edu)

class of_port_features {
public:

	// default constructors
	of_port_features();
	of_port_features(uint32_t raw_value);

	void clear();

	// user-accessible switches
	bool bandwidth_10mb_half_duplex;
	bool bandwidth_10mb_full_duplex;
	bool bandwidth_100mb_half_duplex;
	bool bandwidth_100mb_full_duplex;
	bool bandwidth_1gb_half_duplex;
	bool bandwidth_1gb_full_duplex;
	bool bandwidth_10gb_full_duplex;
	bool medium_copper;
	bool medium_fiber;
	bool auto_negotiation;
	bool pause;
	bool asymmetric_pause;

	void set(uint32_t raw_value);
	uint32_t get() const;

	std::string to_string() const;
};

#endif
