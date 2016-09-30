#include "of_message_vendor.h"

// constructor
of_message_vendor::of_message_vendor() {
	clear();
}

// clears the message
void of_message_vendor::clear() {
	of_message::clear();
	vendor = 0;
//	request.clear();
	msg_type = OFPT_VENDOR;
	msg_size = sizeof(struct ofp_header);
}

// generates a readable form of the vendor message
std::string of_message_vendor::to_string() const {
	char buf[16];
	std::string result = of_message::to_string() + "\nvendor ID: "; 
	sprintf(buf, "%d", vendor);
	result += buf;

	return result;
}

// serializes the message into a sendable form
uint32_t of_message_vendor::serialize(autobuf& dest) const {
	
	uint32_t size_required = sizeof(struct ofp_vendor_header) + request.size();
	dest.create_empty_buffer(size_required, false);

	struct ofp_vendor_header* hdr = (struct ofp_vendor_header*) dest.get_content_ptr_mutable();
	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(size_required);
	hdr->header.xid = htonl(xid);

	hdr->vendor = htonl(vendor);
	memcpy(dest.ptr_offset_mutable(sizeof(struct ofp_vendor_header)), request.get_content_ptr(), request.size());

	return size_required;
}

// deserializes the vendor message
bool of_message_vendor::deserialize(const autobuf& input) {
	struct ofp_vendor_header header;

	clear();
	bool status = of_message::deserialize(input);
	if (!status) {
		goto fail;
	}

	if (msg_size != input.size()) {
		goto fail;
	}

	memcpy(&header, input.get_content_ptr(), sizeof(struct ofp_vendor_header));
	vendor = ntohl(header.vendor);
	if (msg_size > sizeof(ofp_vendor_header)) {
		request.set_content(
		((const char*)input.get_content_ptr()) + 
		sizeof(struct ofp_vendor_header), input.size() -
		sizeof(struct ofp_vendor_header));
	}

	return true;

fail:
	clear();
	return false;
}
