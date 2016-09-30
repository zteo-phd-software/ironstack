#include "of_message_modify_flow.h"
using namespace std;

// constructor
of_message_modify_flow::of_message_modify_flow() {
	clear();
}

// clears the object
void of_message_modify_flow::clear() {
	msg_type = OFPT_FLOW_MOD;
	flow_description.clear();
	command = OFPFC_ADD;
	idle_timeout = 0;
	hard_timeout = 0;
	buffer_id = 0;
	use_out_port = false;
	out_port = (uint16_t) OFPP_NONE;
	flag_send_flow_removal_message = false;
	flag_check_overlap = false;
	flag_emergency_flow = false;
}

// generates a readable form of the message
string of_message_modify_flow::to_string() const {
	char buf[64];
	string result = of_message::to_string() + "\n";

	result += flow_description.to_string();
	result += "\ncommand: ";

	switch (command) {
		case OFPFC_ADD:
			result += "ADD FLOW";
			break;
		case OFPFC_MODIFY:
			result += "MODIFY FLOW";
			break;
		case OFPFC_MODIFY_STRICT:
			result += "MODIFY FLOW STRICT";
			break;
		case OFPFC_DELETE:
			result += "DELETE FLOW";
			break;
		case OFPFC_DELETE_STRICT:
			result += "DELETE FLOW STRICT";
			break;
		default:
			break;
	}

	result += "\nidle timeout: ";
	sprintf(buf, "%u", idle_timeout);
	result += buf;

	result += "\nhard timeout: ";
	sprintf(buf, "%u", hard_timeout);
	result += buf;

	result += "\nbuffer id: ";
	if (buffer_id == (uint32_t) -1)
		sprintf(buf, "-1");
	else
		sprintf(buf, "%u", buffer_id);
	result += buf;

	result += "\nflags: ";
	if (!flag_send_flow_removal_message &&
		!flag_check_overlap &&
		!flag_emergency_flow) {

		result += "NONE";

	} else {
		if (flag_send_flow_removal_message) {
			result += "SEND_REMOVAL_MESSAGE ";
		}
		if (flag_check_overlap) {
			result += "CHECK_OVERLAP ";
		}
		if (flag_emergency_flow) {
			result += "EMERGENCY_FLOW";
		}
	}

	return result;
}

// serializes the message
uint32_t of_message_modify_flow::serialize(autobuf& dest) const {

	uint32_t size_required;
	uint32_t action_list_size = flow_description.action_list.get_serialization_size();
	size_required = sizeof(struct ofp_flow_mod) + action_list_size;

	dest.clear();
	dest.create_empty_buffer(size_required, true);
	struct ofp_flow_mod* hdr = (struct ofp_flow_mod*) dest.get_content_ptr_mutable();
	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(size_required);
	hdr->header.xid = htonl(xid);

	autobuf match_serialized;
	flow_description.criteria.serialize(match_serialized);
	memcpy(&hdr->match, match_serialized.get_content_ptr(), sizeof(struct ofp_match));
	if (match_serialized.size() != sizeof(struct ofp_match)) {
		abort();
	}
	
	hdr->cookie = htobe64(flow_description.cookie);
	hdr->command = htons((uint16_t) command);
	hdr->idle_timeout = htons(idle_timeout);
	hdr->hard_timeout = htons(hard_timeout);
	hdr->priority = htons(flow_description.priority);
	hdr->buffer_id = htonl(buffer_id);
	hdr->out_port = htons(use_out_port ? out_port : (uint16_t) OFPP_NONE);
	hdr->flags |= (flag_send_flow_removal_message ? (uint16_t) OFPFF_SEND_FLOW_REM : 0);
	hdr->flags |= (flag_check_overlap ? (uint16_t) OFPFF_CHECK_OVERLAP : 0);
	hdr->flags |= (flag_emergency_flow ? (uint16_t) OFPFF_EMERG : 0);
	hdr->flags = htons(hdr->flags);

	if (action_list_size > 0) {
		autobuf action_list_serialized;
		action_list_serialized.inherit_shared(dest.ptr_offset_mutable(sizeof(struct ofp_flow_mod)),
			action_list_size, action_list_size);
		flow_description.action_list.serialize(action_list_serialized);
	}

	return size_required;
}

// deserializes the message
bool of_message_modify_flow::deserialize(const autobuf& input) {
	const struct ofp_flow_mod* header = (const struct ofp_flow_mod*) input.get_content_ptr();
	autobuf match_struct;
	uint16_t cmd, flags;

	clear();
	bool status = of_message::deserialize(input);
	if (!status || input.size() < sizeof(struct ofp_match) || msg_type != OFPT_FLOW_MOD)
		goto fail;

	match_struct.inherit_read_only(&header->match, sizeof(struct ofp_match));

	status = flow_description.criteria.deserialize(match_struct);
	if (!status) {
		goto fail;
	}

	flow_description.cookie = header->cookie;
	
	cmd = ntohs(header->command);
	switch (cmd)	{
		case ((uint16_t) OFPFC_ADD):
			command = OFPFC_ADD;
			break;
		case ((uint16_t) OFPFC_MODIFY):
			command = OFPFC_MODIFY;
			break;
		case ((uint16_t) OFPFC_MODIFY_STRICT):
			command = OFPFC_MODIFY_STRICT;
			break;
		case ((uint16_t) OFPFC_DELETE):
			command = OFPFC_DELETE;
			break;
		case ((uint16_t) OFPFC_DELETE_STRICT):
			command = OFPFC_DELETE_STRICT;
			break;
		default:
			goto fail;
	}

	idle_timeout = ntohs(header->idle_timeout);
	hard_timeout = ntohs(header->hard_timeout);
	flow_description.priority = ntohs(header->priority);
	buffer_id = ntohl(header->buffer_id);

	out_port = ntohs(header->out_port);
	use_out_port = (out_port == (uint16_t) OFPP_NONE ? false : true);

	flags = ntohs(header->flags);
	flag_send_flow_removal_message = (flags & (uint16_t) OFPFF_SEND_FLOW_REM) > 0;
	flag_check_overlap = (flags & (uint16_t) OFPFF_CHECK_OVERLAP) > 0;
	flag_emergency_flow = (flags & (uint16_t) OFPFF_EMERG) > 0;

	// action lists not generated for incoming packets
	return true;

fail:
	clear();
	return false;
}
