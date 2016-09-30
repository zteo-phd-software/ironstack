#pragma once

#include <arpa/inet.h>
#include <vector>
#include "of_message.h"
#include "../ironstack_types/openflow_port.h"
#include "../ironstack_types/openflow_switch_features.h"

// openflow switch features message
// switch features and physical ports are described in this message
//
// author: Z. Teo (zteo@cs.cornell.edu)

using namespace std;

// message used to handshake with openflow
class of_message_features_reply : public of_message {
public:
	
	of_message_features_reply();
	virtual ~of_message_features_reply() {};

	virtual void clear();
	virtual string to_string() const;

	// inherited from serializable
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

	// data
	openflow_switch_features  switch_features;
	vector<openflow_port>     physical_ports;
};

