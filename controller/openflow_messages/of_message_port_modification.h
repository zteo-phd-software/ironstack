#pragma once

#include "of_message.h"
#include "../../common/mac_address.h"
#include "../openflow_types/of_port_features.h"
#include "../ironstack_types/openflow_port_config.h"

using namespace std;
class of_message_port_modification : public of_message {
public:
	of_message_port_modification();
	virtual ~of_message_port_modification() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	uint16_t              port_no;
	mac_address           port_mac_address;

	openflow_port_config  new_config;		// set this to the new configuration
																			// note: all flags must change.
																			// if old flags are needed, get them
																			// from the original port info

	bool                  change_features;	// set this flag to make switch
	                                        // take up new features
	of_port_features      features;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
