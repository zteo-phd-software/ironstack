#include "of_message_barrier_request.h"

using namespace std;
// default constructor
of_message_barrier_request::of_message_barrier_request() {
	clear();
}

// clears the message
void of_message_barrier_request::clear() {
	of_message::clear();
	msg_type = OFPT_BARRIER_REQUEST;
}

// generates a readable string
string of_message_barrier_request::to_string() const {
	return of_message::to_string();
}

// serialize method for serializable class
uint32_t of_message_barrier_request::serialize(autobuf& dest) const {
	return of_message::serialize(dest);
}

// should never be called
bool of_message_barrier_request::deserialize(const autobuf& input) {
	abort();
	return false;
}
