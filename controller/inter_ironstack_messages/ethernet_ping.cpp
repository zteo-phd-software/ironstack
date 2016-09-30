#include "ethernet_ping.h"
#include "../gui/output.h"
#include "../../common/common_utils.h"
using namespace ironstack;

// constructor
ethernet_ping::ethernet_ping() {
	msg_type = ironstack::inter_ironstack_message::message_t::ETHERNET_PING;
}

// destructor
ethernet_ping::~ethernet_ping() {}

// generates a readable form of the message
string ethernet_ping::to_string() const {
	return inter_ironstack_message::to_string();
}

// serializes the message
uint32_t ethernet_ping::serialize(autobuf& dest) const {
	dest.create_empty_buffer(sizeof(ironstack_message_hdr));
	ironstack_message_hdr* hdr = (ironstack_message_hdr*) dest.get_content_ptr_mutable();
	hdr->version = VERSION;
	pack_uint16(hdr->msg_type, (uint16_t) msg_type);
	pack_uint32(hdr->xid, xid);
	pack_uint16(hdr->payload_size, 0);

	return dest.size();
}

// deserializes the message
bool ethernet_ping::deserialize(const autobuf& buf) {

	clear();
	if (buf.size() < sizeof(ironstack_message_hdr)) {
		output::log(output::loglevel::BUG, "ethernet_ping::deserialize() -- msg size %zd, failed sanity check.\n", buf.size());
		return false;
	}

	// deserialize the common header
	ironstack_message_hdr* hdr = (ironstack_message_hdr*) buf.get_content_ptr();
	version = hdr->version;
	msg_type = (message_t) unpack_uint16(hdr->msg_type);
	xid = unpack_uint32(hdr->xid);

	// more sanity checks
	if (version != VERSION || msg_type != message_t::ETHERNET_PING) {
		output::log(output::loglevel::BUG, "ethernet_ping::deserialize() -- version %d, msg_type %d, failed sanity check.\n", version, msg_type);
		return false;
	}

	return true;
}
