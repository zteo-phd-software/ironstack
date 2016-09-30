#include "of_message_features_request.h"
using namespace std;

// default constructor
of_message_features_request::of_message_features_request() {
	clear();
}

// clears the message
void of_message_features_request::clear() {
	version = 1;
	msg_type = OFPT_FEATURES_REQUEST;
	msg_size = sizeof(struct ofp_header);
	xid = 0;
}

// returns verbose version of the message type
string of_message_features_request::to_string() const {
	return of_message::to_string();
}

// serializes the features request message
uint32_t of_message_features_request::serialize(autobuf& dest) const {
	return of_message::serialize(dest);
}

// deserializes the features request message
// should never be called
bool of_message_features_request::deserialize(const autobuf& input) {
	abort();
	return false;
}
