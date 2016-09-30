#include "of_message_stats_request.h"
using namespace std;

// constructor
of_message_stats_request_switch_description::of_message_stats_request_switch_description() {
	clear();
}

// clears the object
void of_message_stats_request_switch_description::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REQUEST;
}

// generates a readable form of the message
string of_message_stats_request_switch_description::to_string() const {
	string result =  of_message::to_string();
	result += "\nrequest subtype: SWITCH DESCRIPTION";

	return result;
}

// serializes the message
uint32_t of_message_stats_request_switch_description::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_stats_request), false);
	struct ofp_stats_request* hdr = (struct ofp_stats_request*) dest.get_content_ptr_mutable();

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(sizeof(struct ofp_stats_request));
	hdr->header.xid = htonl(xid);

	hdr->type = htons((uint16_t) OFPST_DESC);
	hdr->flags = 0;

	return sizeof(struct ofp_stats_request);
}

// should not be called
bool of_message_stats_request_switch_description::deserialize(const autobuf& input) {
	abort();
	return false;
}

// constructor
of_message_stats_request_flow_stats::of_message_stats_request_flow_stats() {
	clear();
}

// clears the object
void of_message_stats_request_flow_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REQUEST;
	fields_to_match.clear();
	all_tables = true;
	table_id = 0;

	restrict_out_port = false;
	out_port = (uint16_t) OFPP_NONE;
}

// generates a readable form of the message
string of_message_stats_request_flow_stats::to_string() const {
	char buf[16];
	string result = of_message::to_string();
	result += "\nrequest subtype: FLOW STATS\n";
	result += fields_to_match.to_string();
	result += "\ntable id: ";
	if (all_tables) {
		result += "ALL";
	} else {
		sprintf(buf, "%u", table_id);
		result += buf;
	}

	result += "\nout port: ";
	if (restrict_out_port) {
		sprintf(buf, "%u", out_port);
		result += buf;
	} else {
		result += "ANY";
	}

	return result;
}

// serializes the message
uint32_t of_message_stats_request_flow_stats::serialize(autobuf& dest) const {

	uint32_t size_required = sizeof(struct ofp_stats_request) + sizeof(struct ofp_flow_stats_request);
	dest.create_empty_buffer(size_required, true);
	struct ofp_stats_request* hdr = (struct ofp_stats_request*) dest.get_content_ptr_mutable();
	struct ofp_flow_stats_request* request = (struct ofp_flow_stats_request*) dest.ptr_offset_mutable(sizeof(struct ofp_stats_request));

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(size_required);
	hdr->header.xid = htonl(xid);

	hdr->type = htons((uint16_t) OFPST_FLOW);
	hdr->flags = 0;

	autobuf match_buf;
	match_buf.inherit_shared(&request->match, sizeof(struct ofp_match));
//	autobuf match_buf(&request->match, sizeof(struct ofp_match), 0, false);
	fields_to_match.serialize(match_buf);

	request->table_id = all_tables ? 0xff : table_id;
	request->out_port = htons(restrict_out_port ? out_port : (uint16_t) OFPP_NONE);

	return size_required;
}

// should not be called
bool of_message_stats_request_flow_stats::deserialize(const autobuf& input) {
	abort();
	return false;
}

// constructor
of_message_stats_request_aggregate_stats::of_message_stats_request_aggregate_stats() {
	clear();
}

// clears the object
void of_message_stats_request_aggregate_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REQUEST;
	fields_to_match.clear();
	table_id = 0;
	restrict_out_port = false;
	out_port = (uint16_t) OFPP_NONE;
}

// generates a readable form of the message
string of_message_stats_request_aggregate_stats::to_string() const {
	char buf[16];
	string result = of_message::to_string();
	result += "\nrequest subtype: AGGREGATE STATS";
	result += "\nmatching flow ruleset\n";
	result += fields_to_match.to_string();
	result += "\ntable id: ";
	if (all_tables) {
		result += "ALL";
	} else {
		sprintf(buf, "%u", table_id);
		result += buf;
	}

	result += "\nout port: ";
	if (restrict_out_port) {
		sprintf(buf, "%u", out_port);
		result += buf;
	} else {
		result += "ANY";
	}

	return result;
}

// serializes the message
uint32_t of_message_stats_request_aggregate_stats::serialize(autobuf& dest) const {

	uint32_t size_required = sizeof(struct ofp_stats_request) + sizeof(struct ofp_aggregate_stats_request);
	dest.create_empty_buffer(size_required, false);

	struct ofp_stats_request* hdr = (struct ofp_stats_request*) dest.get_content_ptr_mutable();
	struct ofp_aggregate_stats_request* request = (struct ofp_aggregate_stats_request*) dest.ptr_offset_mutable(sizeof(struct ofp_stats_request));

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(size_required);
	hdr->header.xid = htonl(xid);

	hdr->type = htons((uint16_t)OFPST_AGGREGATE);
	hdr->flags = 0;

	autobuf match_buf;
	match_buf.inherit_shared(request, sizeof(struct ofp_match));
//	autobuf match_buf(request, sizeof(struct ofp_match), 0, false);
	fields_to_match.serialize(match_buf);

	request->table_id = all_tables ? 0xff : table_id;
	request->out_port = htons(restrict_out_port ? out_port : (uint16_t) OFPP_NONE);

	return size_required;
}

// should not be called
bool of_message_stats_request_aggregate_stats::deserialize(const autobuf& input) {
	abort();
	return false;
}

// constructor
of_message_stats_request_table_stats::of_message_stats_request_table_stats() {
	clear();
}

// clears the object
void of_message_stats_request_table_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REQUEST;
}

// generates a readable form of the message
string of_message_stats_request_table_stats::to_string() const {
	string result = of_message::to_string();
	result += "\nrequest subtype: TABLE_STATS";

	return result;
}

// serializes the message
uint32_t of_message_stats_request_table_stats::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_stats_request), false);
	struct ofp_stats_request* hdr = (struct ofp_stats_request*) dest.get_content_ptr_mutable();

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(sizeof(struct ofp_stats_request));
	hdr->header.xid = htonl(xid);

	hdr->type = htons((uint16_t)OFPST_TABLE);
	hdr->flags = 0;

	return sizeof(struct ofp_stats_request);
}

// should not be called
bool of_message_stats_request_table_stats::deserialize(const autobuf& input) {
	abort();
	return false;
}

// constructor
of_message_stats_request_port_stats::of_message_stats_request_port_stats() {
	clear();
}

// clears the object
void of_message_stats_request_port_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REQUEST;
	all_ports = true;
	port = 0;
}

// generates a readable form of the message
string of_message_stats_request_port_stats::to_string() const {
	char buf[16];
	string result = of_message::to_string();
	result += "\nrequest subtype: PORT STATS";
	result += "\nport: ";
	if (all_ports) {
		result += "ALL";
	} else {
		sprintf(buf, "%u", port);
		result += buf;
	}

	return result;
}

// serializes the message
uint32_t of_message_stats_request_port_stats::serialize(autobuf& dest) const {

	uint32_t size_required = sizeof(struct ofp_stats_request) + sizeof(struct ofp_port_stats_request);
	dest.create_empty_buffer(size_required, true);

	struct ofp_stats_request* hdr = (struct ofp_stats_request*) dest.get_content_ptr_mutable();
	struct ofp_port_stats_request* request = (struct ofp_port_stats_request*) dest.ptr_offset_mutable(sizeof(struct ofp_stats_request));

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(size_required);
	hdr->header.xid = htonl(xid);

	hdr->type = htons((uint16_t) OFPST_PORT);
	hdr->flags = 0;
	
	request->port_no = htons(all_ports ? (uint16_t) OFPP_NONE : port);

	return size_required;
}

// should not be called
bool of_message_stats_request_port_stats::deserialize(const autobuf& input) {
	abort();
	return false;
}

// constructor
of_message_stats_request_queue_stats::of_message_stats_request_queue_stats() {
	clear();
}

// clears the object
void of_message_stats_request_queue_stats::clear() {
	of_message::clear();
	msg_type = OFPT_STATS_REQUEST;
	all_ports = true;
	port = 0;

	all_queues = true;
	queue_id = 0;
}

// generates a readable form of the message
string of_message_stats_request_queue_stats::to_string() const {
	char buf[16];
	string result = of_message::to_string();
	result += "\nrequest subtype: QUEUE_STATS";
	result += "\nport: ";
	if (all_ports) {
		result += "ALL";
	} else {
		sprintf(buf, "%u", port);
		result += buf;
	}

	result += "\nqueue id: ";
	if (all_queues) {
		result += "ALL";
	} else {
		sprintf(buf, "%u", queue_id);
		result += buf;
	}

	return result;
}

// serializes the message
uint32_t of_message_stats_request_queue_stats::serialize(autobuf& dest) const {

	uint32_t size_required = sizeof(struct ofp_stats_request) + sizeof(struct ofp_queue_stats_request);
	dest.create_empty_buffer(size_required, false);

	struct ofp_stats_request* hdr = (struct ofp_stats_request*) dest.get_content_ptr_mutable();
	struct ofp_queue_stats_request* request = (struct ofp_queue_stats_request*) dest.ptr_offset_mutable(sizeof(struct ofp_stats_request));

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(size_required);
	hdr->header.xid = htonl(xid);

	hdr->type = htons((uint16_t) OFPST_QUEUE);
	hdr->flags = 0;

	request->port_no = htons(all_ports ? (uint16_t) OFPP_ALL : port);
	request->queue_id = htonl(all_queues ? (uint32_t) OFPQ_ALL : queue_id);

	return size_required;
}

// should not be called
bool of_message_stats_request_queue_stats::deserialize(const autobuf& input) {
	abort();
	return false;
}
