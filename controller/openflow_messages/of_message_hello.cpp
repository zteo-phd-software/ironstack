#include "of_message_hello.h"
using namespace std;

// constructor
of_message_hello::of_message_hello() {
	clear();
}

// clears the message
void of_message_hello::clear() {
	version = 1;
	msg_type = OFPT_HELLO;
	msg_size = sizeof(struct ofp_header);
	xid = 0;
}

// returns a verbose version of this message
string of_message_hello::to_string() const {
	return of_message::to_string();
}

// serializes the hello message
uint32_t of_message_hello::serialize(autobuf& dest) const{
	return of_message::serialize(dest);
}

// deserializes the hello message
bool of_message_hello::deserialize(const autobuf& input) {
	clear();
	bool status = of_message::deserialize(input);
	if (!status) {
		goto fail;
	}

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	// sanity checks for message deserialization to hello type
	if (msg_size != sizeof(struct ofp_header) ||
		msg_type != OFPT_HELLO) {
		goto fail;
	}
	#endif

	// hello message type deserialization OK
	return true;

fail:
	clear();
	return false;
}
