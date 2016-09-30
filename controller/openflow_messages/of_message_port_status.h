#pragma once

#include <string>
#include <arpa/inet.h>
#include "../ironstack_types/openflow_port.h"
#include "of_message.h"

using namespace std;
class of_message_port_status : public of_message {
public:

	// constructors
	of_message_port_status();
	virtual ~of_message_port_status() {};

	// user-accessible fields
	enum ofp_port_reason  reason;
	openflow_port         description;

	// universal message functions
	virtual void clear();
	virtual string to_string() const;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
