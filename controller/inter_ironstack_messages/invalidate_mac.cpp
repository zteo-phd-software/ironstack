#include "../../common/common_utils.h"
#include "invalidate_mac.h"
#include "../gui/output.h"
using namespace ironstack;

// constructor
invalidate_mac::invalidate_mac() {
	msg_type = inter_ironstack_message::message_t::INVALIDATE_MAC;
}

// destructor
invalidate_mac::~invalidate_mac() {}

// clears all fields in the message
void invalidate_mac::clear() {
	addresses_to_purge.clear();
}

// generates a readable form of the message
string invalidate_mac::to_string() const {
	string result = inter_ironstack_message::to_string();
	result += "MAC addresses to invalidate: ";
	char buf[16];
	sprintf(buf, "%zd", addresses_to_purge.size());
	result += buf;

	for (const auto& address : addresses_to_purge) {
		result += "\n";
		result += address.to_string();
	}

	return result;
}

// serializes the message for delivery
uint32_t invalidate_mac::serialize(autobuf& dest) const {

	uint32_t entries = addresses_to_purge.size();
	uint32_t bytes_needed = sizeof(ironstack_message_hdr)+6*entries;
	dest.create_empty_buffer(bytes_needed);
	ironstack_message_hdr* hdr = (ironstack_message_hdr*) dest.get_content_ptr_mutable();

	// setup header
	hdr->version = VERSION;
	pack_uint16(hdr->msg_type, (uint16_t) msg_type);
	pack_uint32(hdr->xid, xid);
	pack_uint16(hdr->payload_size, 6*entries);

	auto iterator = addresses_to_purge.begin();
	for (uint32_t counter = 0; counter < entries; ++counter) {
		iterator->get(hdr->data + counter*6);
		++iterator;
	}

	return bytes_needed;
}

// deserializes the message
bool invalidate_mac::deserialize(const autobuf& buf) {

	clear();

	// sanity check
	if (buf.size() < sizeof(ironstack_message_hdr) ||
		((buf.size()-sizeof(ironstack_message_hdr)) % 6 != 0)) {
		output::log(output::loglevel::BUG, "invalidate_mac::deserialize() -- msg size = %zd, failed sanity check.\n", buf.size());
		return false;
	}

	// deserialize common header
	ironstack_message_hdr* hdr = (ironstack_message_hdr*) buf.get_content_ptr();
	version = hdr->version;
	msg_type = (message_t) unpack_uint16(hdr->msg_type);
	xid = unpack_uint32(hdr->xid);

	// more sanity checks
	if (version != VERSION || msg_type != message_t::INVALIDATE_MAC) {
		output::log(output::loglevel::BUG, "invalidate_mac::deserialize() -- version %d, msg_type %d, failed sanity check.\n", version, msg_type);
		return false;
	}

	uint32_t entries = (buf.size() - sizeof(ironstack_message_hdr)) / 6;
	mac_address current_mac;
	for (uint32_t counter = 0; counter < entries; ++counter) {
		current_mac.set_from_network_buffer(hdr->data + counter*6);
		addresses_to_purge.push_back(current_mac);
	}

	return true;
}
