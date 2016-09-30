#include "of_message_stats_reply.h"
#include "../gui/output.h"
using namespace std;

// constructor
of_message_stats_reply_switch_description::of_message_stats_reply_switch_description() {
	clear();
}

// clears the object
void of_message_stats_reply_switch_description::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REPLY;
	stats_type = of_message_stats_reply::stats_t::SWITCH_DESCRIPTION;
	more_to_follow = false;
	description.clear();
}

// generates a readable form of the message
string of_message_stats_reply_switch_description::to_string() const {
	string result = of_message::to_string();
	result += string("\n--- switch description ---\n");
	if (more_to_follow) {
		string result = description.to_string();
		result += string("\n[more information to follow]");
	} else {
		result = description.to_string();
	}

	return result;
}

// should not be called (if you just want to serialize the description,
// call description.serialize())
uint32_t of_message_stats_reply_switch_description::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message into a usable form
bool of_message_stats_reply_switch_description::deserialize(const autobuf& input) {

	clear();
	const struct ofp_stats_reply* hdr = (const struct ofp_stats_reply*) input.get_content_ptr();
	bool status = of_message::deserialize(input);
	autobuf description_buf;

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	uint16_t hdr_type;
	if (!status || msg_type != OFPT_STATS_REPLY || input.size() < sizeof(struct ofp_stats_reply)) {
		goto fail;
	}

	hdr_type = ntohs(hdr->type);
	if (hdr_type != (uint16_t) OFPST_DESC || input.size() < (sizeof(struct ofp_stats_reply) + sizeof(ofp_desc_stats))) {
		goto fail;
	}
	#endif

	more_to_follow = ((ntohs(hdr->flags) & 0x0001) > 0);
	
	description_buf.inherit_read_only(hdr->body, sizeof(struct ofp_desc_stats));
	if (!description.deserialize(description_buf)) {
		goto fail;
	}
	return true;

fail:
	clear();
	return false;
}

// constructor
of_message_stats_reply_flow_stats::of_message_stats_reply_flow_stats() {
	clear();
}

// clears the object
void of_message_stats_reply_flow_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REPLY;
	stats_type = of_message_stats_reply::stats_t::FLOW_STATS;
	flow_stats.clear();
	more_to_follow = false;
}

// generates a readable form of the message
string of_message_stats_reply_flow_stats::to_string() const {
	string result = of_message::to_string();
	result += string("\n--- flow stats ---");
	if (flow_stats.size() == 0) {
		result += string("\nno more flows.");
		return result;
	} else {
		for (const auto& flow : flow_stats) {
			result += string("\n\n");
			result += flow.to_string();
		}
	}

	if (more_to_follow) {
		result += "\n[more to follow]";
	}
	return result;
}

// should not be called. if you wish to serialize the flow description and stats,
// call the serialize() function on each flow
uint32_t of_message_stats_reply_flow_stats::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message into a usable form
bool of_message_stats_reply_flow_stats::deserialize(const autobuf& input) {

	clear();
	const struct ofp_stats_reply* hdr = (const struct ofp_stats_reply*) input.get_content_ptr();
	bool status = of_message::deserialize(input);

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status || msg_type != OFPT_STATS_REPLY || input.size() < sizeof(struct ofp_stats_reply)) {
		clear();
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);

	if (hdr_type != (uint16_t) OFPST_FLOW) {
		clear();
		return false;
	}
	#endif

	if (input.size()-sizeof(struct ofp_stats_reply) == 0) {
		// no flows to deserialize
		return true;
	}

	more_to_follow = ((ntohs(hdr->flags) & 0x0001) > 0);

	// construct main autobuf to hold the flow array
	autobuf flow_buf;
	flow_buf.inherit_read_only(input.ptr_offset_const(sizeof(struct ofp_stats_reply)), input.size()-sizeof(struct ofp_stats_reply));
	autobuf one_flow;
	openflow_flow_description_and_stats flow;
	uint32_t read_offset = 0;
	uint32_t size_to_read = 0;

	while (read_offset < flow_buf.size()) {
		#ifndef __NO_OPENFLOW_SAFETY_CHECKS
		if (flow_buf.size()-read_offset < sizeof(struct ofp_flow_stats)) {
			output::log(output::loglevel::BUG, "of_message_stats_reply_flow_stats::deserialize() -- buf size < sizeof(body).\n");
			goto fail;
		}
		#endif

		size_to_read = ntohs(((struct ofp_flow_stats*) flow_buf.ptr_offset_const(read_offset))->length);

		#ifndef __NO_OPENFLOW_SAFETY_CHECKS
		if (read_offset+size_to_read > flow_buf.size()) {
			output::log(output::loglevel::BUG, "of_message_stats_reply_flow_stats::deserialize() error -- size to read > flow_buf.size()\n");
			goto fail;
		}
		#endif

		one_flow.inherit_read_only(flow_buf.ptr_offset_const(read_offset), size_to_read);
		if (!flow.deserialize(one_flow)) {
			output::log(output::loglevel::BUG, "of_message_stats_reply_flow_stats::deserialize() error -- unable to deserialize single flow.\n");
			goto fail;
		}
		flow_stats.push_back(flow);
		read_offset += size_to_read;
	}

	return true;

fail:
	clear();
	return false;
}

// constructor
of_message_stats_reply_aggregate_stats::of_message_stats_reply_aggregate_stats() {
	clear();
}

// clears the object
void of_message_stats_reply_aggregate_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REPLY;
	stats_type = of_message_stats_reply::stats_t::AGGREGATE_STATS;
	more_to_follow = false;
	aggregate_stats.clear();
}

// generates a readable form of the message
string of_message_stats_reply_aggregate_stats::to_string() const {
	string result = of_message::to_string();
	result += string("\n--- aggregate stats ---\n");
	result += aggregate_stats.to_string();
	if (more_to_follow) {
		result += string("\n[more to follow]");
	}
	return result;
}

// should not be called. if you wish to serialize aggregate stats, call the serialize
// function directly on the aggregate_stats object.
uint32_t of_message_stats_reply_aggregate_stats::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message into a usable form
bool of_message_stats_reply_aggregate_stats::deserialize(const autobuf& input) {
	clear();
	bool status = of_message::deserialize(input);
	const struct ofp_stats_reply* hdr = (const struct ofp_stats_reply*) input.get_content_ptr();
	uint16_t hdr_type;
	autobuf aggregate_stats_buf;

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status ||
		msg_type != OFPT_STATS_REPLY ||
		input.size() < sizeof(struct ofp_stats_reply) + sizeof(struct ofp_aggregate_stats_reply)) {
		goto fail;
	}

	hdr_type = ntohs(hdr->type);
	if (hdr_type != (uint16_t) OFPST_AGGREGATE) {
		goto fail;
	}
	#endif

	more_to_follow = (ntohs(hdr->flags) & 0x0001) > 0;
	aggregate_stats_buf.inherit_read_only(hdr->body, sizeof(struct ofp_aggregate_stats_reply));
	if (!aggregate_stats.deserialize(aggregate_stats_buf)) {
		return false;
	}
	return true;

fail:
	clear();
	return false;
}

// constructor
of_message_stats_reply_table_stats::of_message_stats_reply_table_stats() {
	clear();
}

// clears the object
void of_message_stats_reply_table_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REPLY;
	stats_type = of_message_stats_reply::stats_t::TABLE_STATS;
	more_to_follow = false;
	table_stats.clear();
}

// generates a readable form of the message
string of_message_stats_reply_table_stats::to_string() const {
	string result = of_message::to_string();
	result += string("\n--- table stats ---");
	if (table_stats.empty()) {
		result += string("\nno more tables.");
	} else {
		for (const auto& one_table_stats : table_stats) {
			result += string("\n\n");
			result += one_table_stats.to_string();
		}
	}

	if (more_to_follow) {
		result += string("\n[more to follow]");
	}
	return result;
}

// should not be called
uint32_t of_message_stats_reply_table_stats::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message into a usable form
bool of_message_stats_reply_table_stats::deserialize(const autobuf& input) {

	clear();

	bool status = of_message::deserialize(input);
	const struct ofp_stats_reply* hdr = (const struct ofp_stats_reply*) input.get_content_ptr();
	uint16_t hdr_type;
	autobuf table_stats_buf;
	autobuf single_table_stats_buf;
	openflow_table_stats single_table_stats;
	uint32_t read_offset = 0;
	uint32_t array_size = 0;

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status || msg_type != OFPT_STATS_REPLY || input.size() < sizeof(struct ofp_stats_reply)) {
		goto fail;
	}

	hdr_type = ntohs(hdr->type);
	if (hdr_type != (uint16_t) OFPST_TABLE) {
		goto fail;
	}
	#endif

	more_to_follow = (ntohs(hdr->flags) & 0x0001) > 0;
	array_size = input.size() - sizeof(struct ofp_stats_reply);
	table_stats_buf.inherit_read_only(hdr->body, array_size);
	
	while(read_offset < array_size) {

		#ifndef __NO_OPENFLOW_SAFETY_CHECKS
		if (array_size - read_offset < sizeof(struct ofp_table_stats)) {
			goto fail;
		}
		#endif

		single_table_stats_buf.inherit_read_only(table_stats_buf.ptr_offset_const(read_offset), sizeof(struct ofp_table_stats));
		if (!single_table_stats.deserialize(single_table_stats_buf)) {
			goto fail;
		}
		table_stats.push_back(single_table_stats);
		read_offset += sizeof(struct ofp_table_stats);
	}
	return true;

fail:
	clear();
	return false;
}

// constructor
of_message_stats_reply_port_stats::of_message_stats_reply_port_stats() {
	clear();
}

// clears the object
void of_message_stats_reply_port_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REPLY;
	stats_type = of_message_stats_reply::stats_t::PORT_STATS;
	more_to_follow = false;
	port_stats.clear();
}

// generates a readable form of the message
string of_message_stats_reply_port_stats::to_string() const {
	string result = of_message::to_string();
	result += string("\n--- port stats	---");
	if (port_stats.empty()) {
		result += string("\nno more ports.");
	} else {
		for (const auto& port : port_stats) {
			result += string("\n\n");
			result += port.to_string();
		}
	}

	if (more_to_follow) {
		result += string("\n[more to follow]");
	}

	return result;
}

// should not be called. if you wish to serialize the port stats, call the serialize
// function directly on the object itself.
uint32_t of_message_stats_reply_port_stats::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message into a usable form
bool of_message_stats_reply_port_stats::deserialize(const autobuf& input) {
	clear();

	bool status = of_message::deserialize(input);
	const struct ofp_stats_reply* hdr = (const struct ofp_stats_reply*) input.get_content_ptr();
	uint16_t hdr_type;
	autobuf port_stats_array_buf;
	autobuf port_stats_buf;
	openflow_port_stats single_port_stats;
	uint32_t array_size = 0;
	uint32_t read_offset = 0;

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status || msg_type != OFPT_STATS_REPLY || input.size() < sizeof(struct ofp_stats_reply)) {
		goto fail;
	}

	hdr_type = ntohs(hdr->type);
	if (hdr_type != (uint16_t) OFPST_PORT) {
		goto fail;
	}
	#endif

	more_to_follow = (ntohs(hdr->flags) & 0x0001) > 0;
	array_size = input.size()-sizeof(struct ofp_stats_reply);
	if (array_size == 0) {
		return true;
	}

	port_stats_array_buf.inherit_read_only(hdr->body, array_size);

	while (read_offset < array_size) {
		
		#ifndef __NO_OPENFLOW_SAFETY_CHECKS
		if (array_size - read_offset < sizeof(struct ofp_port_stats)) {
			output::log(output::loglevel::BUG, "of_message_stats_reply_port_stats::deserialize() port array too small.\n");
			goto fail;
		}
		#endif

		port_stats_buf.inherit_read_only(port_stats_array_buf.ptr_offset_const(read_offset), sizeof(struct ofp_port_stats));
		if (!single_port_stats.deserialize(port_stats_buf)) {
			output::log(output::loglevel::BUG, "of_message_stats_reply_port_stats::deserialize() cannot deserialize port.\n");
			goto fail;
		}
		port_stats.push_back(single_port_stats);
		read_offset += sizeof(struct ofp_port_stats);
	}
	return true;

fail:
	clear();
	return false;
}

// constructor
of_message_stats_reply_queue_stats::of_message_stats_reply_queue_stats() {
	clear();
}

// clears the object
void of_message_stats_reply_queue_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REPLY;
	stats_type = of_message_stats_reply::stats_t::QUEUE_STATS;
	more_to_follow = false;
	queue_stats.clear();
}

// generates a readable form of the message
string of_message_stats_reply_queue_stats::to_string() const {
	string result = of_message::to_string();
	result += string("\n--- queue stats ---");

	if (queue_stats.empty()) {
		result += string("\nno more queues.");
	} else {
		for (const auto& stats : queue_stats) {
			result += string("\n\n");
			result += stats.to_string();
		}
	}

	if (more_to_follow) {
		result += string("\n[more to follow]");
	}

	return result;
}

// should not be called. if you wish to serialize individual queue stats, call the
// serialize function on the openflow_queue_stats object.
uint32_t of_message_stats_reply_queue_stats::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message into a usable form
bool of_message_stats_reply_queue_stats::deserialize(const autobuf& input) {
	clear();
	bool status = of_message::deserialize(input);
	const struct ofp_stats_reply* hdr = (const struct ofp_stats_reply*) input.get_content_ptr();
	uint16_t hdr_type;
	autobuf queue_stats_array_buf;
	autobuf queue_stats_buf;
	openflow_queue_stats single_queue_stats;
	uint32_t read_offset = 0;
	uint32_t array_size = 0;

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status || msg_type != OFPT_STATS_REPLY || input.size() < sizeof(struct ofp_stats_reply)) {
		goto fail;
	}
	
	hdr_type = ntohs(hdr->type);
	if (hdr_type != (uint16_t) OFPST_QUEUE) {
		goto fail;
	}
	#endif

	more_to_follow = (ntohs(hdr->flags) & 0x0001) > 0;
	array_size = input.size() - sizeof(struct ofp_stats_reply);
	queue_stats_array_buf.inherit_read_only(hdr->body, array_size);

	while (read_offset < array_size) {

		#ifndef __NO_OPENFLOW_SAFETY_CHECKS
		if (array_size - read_offset < sizeof(struct ofp_queue_stats)) {
			goto fail;
		}
		#endif

		queue_stats_buf.inherit_read_only(queue_stats_array_buf.ptr_offset_const(read_offset), sizeof(struct ofp_queue_stats));
		if (!single_queue_stats.deserialize(queue_stats_buf)) {
			goto fail;
		}
		queue_stats.push_back(single_queue_stats);
		read_offset += sizeof(struct ofp_queue_stats);
	}

	return true;

fail:
	clear();
	return false;

}
