#pragma once

#include <arpa/inet.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "../utils/of_common_utils.h"
#include "../../common/openflow.h"
#include "../../common/preallocated.h"

// base message class
// author: Z. Teo (zteo@cs.cornell.edu)

using namespace std;
class of_message {
public:

	// constructors
	of_message();
	virtual ~of_message() {};

	// universal message functions
	virtual void clear();
	virtual string to_string() const;

	// get the message type in string form
	string get_message_type_string() const;

	// message header
	uint8_t       version;
	enum ofp_type	msg_type;
	uint16_t      msg_size;
	uint32_t      xid;

	// for serialization
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

