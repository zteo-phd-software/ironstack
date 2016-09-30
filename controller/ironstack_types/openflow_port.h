#pragma once

#include <string>
#include <arpa/inet.h>
#include "openflow_port_config.h"
#include "../../common/openflow.h"
#include "../../common/mac_address.h"
#include "../openflow_types/of_port_state.h"
#include "../openflow_types/of_port_features.h"
using namespace std;

class openflow_port {
public:

	// constructors
	openflow_port();
	openflow_port(const struct ofp_phy_port& input);

	// resets the openflow port object
	void clear();

	// used to quickly check if the port is up or down
	bool is_up() const { return ( !state.port_link_down && !config.port_down); }

	// publicly accessible fields
	uint16_t             port_number;
	mac_address          dl_addr;
	string               name;

	openflow_port_config config;							// really, the only useful info here is the port up/down state
	of_port_state        state;								// again, the only useful info here is the port link state
	of_port_features     current_features;
	of_port_features     advertised_features;
	of_port_features     supported_features;
	of_port_features     peer_features;

	// support functions
	void   set(const struct ofp_phy_port& input);
	string to_string() const;
};

