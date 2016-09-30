#include "of_message_flow_removed.h"
using namespace std;

// constructor
of_message_flow_removed::of_message_flow_removed() {
	clear();
}

// clears the object
void of_message_flow_removed::clear() {
	msg_type = OFPT_FLOW_REMOVED;
	match.clear();
	cookie = 0;
	priority = 0;
	reason_idle_timeout = false;
	reason_hard_timeout = false;
	reason_manually_removed = false;
	duration_alive_sec = 0;
	duration_alive_nsec = 0;
	original_idle_timeout = 0;
	packet_count = 0;
	byte_count = 0;
}

// generates a readable version of the object
string of_message_flow_removed::to_string() const {
	char buf[128];
	std::string result = of_message::to_string();
	result += "\n" + match.to_string();
	result += "\ncookie id: ";
	sprintf(buf, "%" PRIu64, cookie);
	result += buf;
	result += "\npriority: ";
	sprintf(buf, "%u", priority);
	result += buf;
	result += "\nreason for timeout: ";
	if (reason_idle_timeout) {
		result += "idle timeout";
	} else if (reason_hard_timeout) {
		result += "hard timeout";
	} else if (reason_manually_removed) {
		result += "manually removed";
	}	else {
		result += "unknown";
	}
	result += "\nflow alive: ";
	sprintf(buf, "%us : %uns", duration_alive_sec, duration_alive_nsec);
	result += buf;
	result += "\noriginal idle timeout: ";
	sprintf(buf, "%u", original_idle_timeout);
	result += buf;
	result += "\npacket count: ";
	sprintf(buf, "%" PRIu64, packet_count);
	result += buf;
	result += "\nbyte count: ";
	sprintf(buf, "%" PRIu64, byte_count);
	result += buf;

	return result;
}

// should never be called
uint32_t of_message_flow_removed::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message
bool of_message_flow_removed::deserialize(const autobuf& input) {
	clear();
	const struct ofp_flow_removed* header = (const struct ofp_flow_removed*) input.get_content_ptr();
	bool status = of_message::deserialize(input);
	autobuf match_rules;
	if (!status || msg_type != OFPT_FLOW_REMOVED ||
		msg_size != sizeof(struct ofp_flow_removed)) {

		goto fail;
	}

	match_rules.inherit_read_only(input.ptr_offset_const(sizeof(struct ofp_header)),
		sizeof(struct ofp_match));

	if (!match.deserialize(match_rules)) {
		goto fail;
	}

	cookie = be64toh(header->cookie);
	priority = ntohs(header->priority);

	switch (header->reason) {
		case ((uint8_t) OFPRR_IDLE_TIMEOUT):
			reason_idle_timeout = true;
			break;

		case ((uint8_t) OFPRR_HARD_TIMEOUT):
			reason_hard_timeout = true;
			break;

		case ((uint8_t) OFPRR_DELETE):
			reason_manually_removed = true;
			break;
	}

	duration_alive_sec = ntohl(header->duration_sec);
	duration_alive_nsec = ntohl(header->duration_nsec);

	packet_count = be64toh(header->packet_count);
	byte_count = be64toh(header->byte_count);

	return true;

fail:
	clear();
	return false;

}
