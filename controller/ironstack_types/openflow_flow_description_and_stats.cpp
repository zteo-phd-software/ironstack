#include "openflow_flow_description_and_stats.h"

// serializes the flow description and stats
uint32_t openflow_flow_description_and_stats::serialize(autobuf& dest) const {

	autobuf actions_buf;
	flow_description.action_list.serialize(actions_buf);

	dest.create_empty_buffer(sizeof(struct ofp_flow_stats) + actions_buf.size());
	struct ofp_flow_stats* ptr = (struct ofp_flow_stats*)
		dest.get_content_ptr_mutable();

	ptr->length = htons(dest.size());
	ptr->table_id = table_id;
	
	autobuf match_buf;
	match_buf.inherit_shared(&ptr->match, sizeof(struct ofp_match));
	flow_description.criteria.serialize(match_buf);

	ptr->duration_sec = htonl(duration_alive_sec);
	ptr->duration_nsec = htonl(duration_alive_nsec);
	ptr->priority = htons(flow_description.priority);
	ptr->idle_timeout = htons(duration_to_idle_timeout);
	ptr->hard_timeout = htons(duration_to_hard_timeout);
	ptr->cookie = htobe64(flow_description.cookie);
	ptr->packet_count = htobe64(packet_count);
	ptr->byte_count = htobe64(byte_count);

	memcpy(ptr->actions, actions_buf.get_content_ptr(), actions_buf.size());

	return dest.size();
}

// deserializes the object
bool openflow_flow_description_and_stats::deserialize(const autobuf& input) {

	clear();

	uint32_t len;
	autobuf action_list_buf;
	autobuf match_buf;
	const struct ofp_flow_stats* ptr = (const struct ofp_flow_stats*)
		input.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (input.size() < sizeof(struct ofp_flow_stats)) {
		goto fail;
	}
	#endif

	len = ntohs(ptr->length);
	table_id = ptr->table_id;

	match_buf.inherit_read_only(&ptr->match, sizeof(struct ofp_match));
	if (!flow_description.criteria.deserialize(match_buf)) {
		goto fail;
	}

	duration_alive_sec = ntohl(ptr->duration_sec);
	duration_alive_nsec = ntohl(ptr->duration_nsec);
	flow_description.priority = ntohs(ptr->priority);
	duration_to_idle_timeout = ntohs(ptr->idle_timeout);
	duration_to_hard_timeout = ntohs(ptr->hard_timeout);
	flow_description.cookie = be64toh(ptr->cookie);
	packet_count = be64toh(ptr->packet_count);
	byte_count = be64toh(ptr->byte_count);

	action_list_buf.inherit_read_only(ptr->actions, len-sizeof(struct ofp_flow_stats));
	if (!flow_description.action_list.deserialize(action_list_buf)) {
		goto fail;
	}

	return true;

fail:
	clear();
	return false;
}
