#include "inter_ironstack_message.h"
#include "../../common/common_utils.h"
#include <stdio.h>
using namespace ironstack;

// default constructor
inter_ironstack_message::inter_ironstack_message() {
	inter_ironstack_message::clear();
}

// default destructor
inter_ironstack_message::~inter_ironstack_message() { }

// clears the message
void inter_ironstack_message::clear() {
	version = VERSION;
	msg_type = message_t::UNKNOWN;
	xid = 0;
}

// generates a readable version of the message
string inter_ironstack_message::to_string() const {
	string result;
	char buf[128];
	sprintf(buf, "inter-ironstack message v%d.\nxid %u\nmsg type: ", version, xid);
	result += buf;
	result += get_message_type_string();

	return result;
}

// returns the message type in string form
string inter_ironstack_message::get_message_type_string() const {
	switch (msg_type) {
		case message_t::ETHERNET_PING:
			return string("ETHERNET_PING");

		case message_t::ETHERNET_PONG:
			return string("ETHERNET_PONG");

		case message_t::INVALIDATE_MAC:
			return string("INVALIDATE_MAC");

		default:
			return string("UNKNOWN");
	}
}
