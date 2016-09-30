#include "of_message_barrier_reply.h"

using namespace std;

// default constructor
of_message_barrier_reply::of_message_barrier_reply() {
	clear();
}

// clears the message
void of_message_barrier_reply::clear() {
	of_message::clear();
	msg_type = OFPT_BARRIER_REPLY;
}

// generates a readable string
string of_message_barrier_reply::to_string() const {
	return of_message::to_string();
}

// should never be called
uint32_t of_message_barrier_reply::serialize(autobuf& dest) const {

	// should never be called
	abort();
	return 0;
}

// deserializes the barrier message
bool of_message_barrier_reply::deserialize(const autobuf& input) {
	bool status = of_message::deserialize(input);

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status || msg_type != OFPT_BARRIER_REPLY) {
		clear();
		return false;
	}
	#endif

	return true;
}
