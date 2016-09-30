#pragma once

#include <string>
#include "of_message.h"

// openflow echo reply message
// author: Z. Teo (zteo@cs.cornell.edu)
//
// classes: of_message_echo_reply

using namespace std;
class of_message_echo_reply : public of_message {
public:
	// constructor
	of_message_echo_reply();
	virtual ~of_message_echo_reply() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible portion
	autobuf data;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

