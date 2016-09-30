#include "openflow_aggregate_stats.h"

uint32_t openflow_aggregate_stats::serialize(autobuf& dest) const {
	dest.create_empty_buffer(sizeof(struct ofp_aggregate_stats_reply), true);
	struct ofp_aggregate_stats_reply* ptr = (struct ofp_aggregate_stats_reply*)
		dest.get_content_ptr_mutable();
	ptr->packet_count = htobe64(packet_count);
	ptr->byte_count = htobe64(byte_count);
	ptr->flow_count = htobe64(flow_count);

	return dest.size();
}

bool openflow_aggregate_stats::deserialize(const autobuf& input) {
	clear();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (input.size() < sizeof(struct ofp_aggregate_stats_reply)) {
		return false;
	}
	#endif

	const struct ofp_aggregate_stats_reply* ptr = (const struct ofp_aggregate_stats_reply*)
		input.get_content_ptr();
	packet_count = be64toh(ptr->packet_count);
	byte_count = be64toh(ptr->byte_count);
	flow_count = be64toh(ptr->flow_count);

	return true;
}
