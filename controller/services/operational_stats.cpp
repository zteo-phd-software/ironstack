#include "operational_stats.h"
#include "../gui/output.h"
#include "../openflow_messages/of_message_stats_request.h"
#include "../hal/hal_transaction.h"

// initializes the service. asks for an update
bool operational_stats::init() {
	output::log(output::loglevel::INFO, "initializing operational stats.\n");
	lock_guard<mutex> g(lock);
	update_all_stats();
	initialized = true;
	aggregate_stats_xid = 0;
	flow_stats_completed = true;
	flow_stats_xid = 0;
	table_stats_completed = true;
	table_stats_xid = 0;
	port_stats_completed = true;
	port_stats_xid = 0;
	queue_stats_completed = true;
	queue_stats_xid = 0;

	return true;
}

// called after the controller is online
bool operational_stats::init2() {
	return true;
}

// shuts down the service. does not reset data (so it can continue to be inspected)
void operational_stats::shutdown() {
	lock_guard<mutex> g(lock);
	initialized = false;
}

// shortcut function to call all updater functions
bool operational_stats::update_all_stats() {
	return update_aggregate_stats() &&
		update_table_stats() &&
		update_flow_stats() &&
		update_port_stats() &&
		update_queue_stats();
}

// call HAL to update aggregate stats
bool operational_stats::update_aggregate_stats() {
	if (!initialized) return false;

	shared_ptr<of_message_stats_request_aggregate_stats> req(new of_message_stats_request_aggregate_stats());
	req->fields_to_match.wildcard_all();
	req->all_tables = true;
	req->restrict_out_port = false;
	shared_ptr<hal_transaction> transaction(new hal_transaction(req, false));
	controller->enqueue_transaction(transaction);

	return true;
}

// call HAL to update table stats
bool operational_stats::update_table_stats() {
	if (!initialized) return false;

	shared_ptr<of_message_stats_request_table_stats> req(new of_message_stats_request_table_stats());
	shared_ptr<hal_transaction> transaction(new hal_transaction(req, false));
	controller->enqueue_transaction(transaction);

	return true;
}

// call HAL to update flow stats
bool operational_stats::update_flow_stats() {
	if (!initialized) return false;

	shared_ptr<of_message_stats_request_flow_stats> req(new of_message_stats_request_flow_stats());
	req->fields_to_match.wildcard_all();
	req->all_tables = true;
	req->restrict_out_port = false;
	shared_ptr<hal_transaction> transaction(new hal_transaction(req, false));
	controller->enqueue_transaction(transaction);

	return true;
}

// call HAL to update port stats
bool operational_stats::update_port_stats() {
	if (!initialized) return false;

	shared_ptr<of_message_stats_request_port_stats> req(new of_message_stats_request_port_stats());
	req->all_ports = true;
	shared_ptr<hal_transaction> transaction(new hal_transaction(req, false));
	controller->enqueue_transaction(transaction);

	return true;
}

// call HAL to update queue stats
bool operational_stats::update_queue_stats() {
	if (!initialized) return false;

	shared_ptr<of_message_stats_request_queue_stats> req(new of_message_stats_request_queue_stats());
	req->all_ports = true;
	req->all_queues = true;
	shared_ptr<hal_transaction> transaction(new hal_transaction(req, false));
	controller->enqueue_transaction(transaction);

	return true;
}

// updates the aggregate stats from the switch
void operational_stats::update_handler(const shared_ptr<of_message_stats_reply_aggregate_stats>& msg) {
	lock_guard<mutex> g(lock);
	aggregate_stats = msg->aggregate_stats;
	aggregate_stats_xid = msg->xid;
}

// updates table stats from the switch
void operational_stats::update_handler(const shared_ptr<of_message_stats_reply_table_stats>& msg) {
	lock_guard<mutex> g(lock);

	if (table_stats_completed || msg->xid > table_stats_xid) {

		// reset table and put in fresh contents
		table_stats_shadow = msg->table_stats;
		table_stats_xid = msg->xid;
		table_stats_completed = !msg->more_to_follow;

	} else if (!table_stats_completed && msg->xid == table_stats_xid) {

		// this is a continuation message. append to table stats
		for (const auto& it : msg->table_stats) {
			table_stats_shadow.push_back(it);
		}
		table_stats_completed = !msg->more_to_follow;
	}

	if (table_stats_completed) {
		table_stats = table_stats_shadow;
	}
}

// updates flow stats from the switch
void operational_stats::update_handler(const shared_ptr<of_message_stats_reply_flow_stats>& msg) {
	lock_guard<mutex> g(lock);

	if (flow_stats_completed ||
		msg->xid > flow_stats_xid) {
		
		// reset table and put in fresh contents
		flow_stats_shadow = msg->flow_stats;
		flow_stats_xid = msg->xid;
		flow_stats_completed = !msg->more_to_follow;

	} else if (!flow_stats_completed && msg->xid == flow_stats_xid) {
		
		// continuation message.
		for (const auto& it : msg->flow_stats) {
			flow_stats_shadow.push_back(it);
		}
		flow_stats_completed = !msg->more_to_follow;
	}

	if (flow_stats_completed) {

		flow_stats = flow_stats_shadow;
	}
}

// updates port stats from the switch
void operational_stats::update_handler(const shared_ptr<of_message_stats_reply_port_stats>& msg) {
	lock_guard<mutex> g(lock);

	if (port_stats_completed || msg->xid > port_stats_xid) {

		// reset table and put in fresh contents
		port_stats_shadow = msg->port_stats;
		port_stats_xid = msg->xid;
		port_stats_completed = !msg->more_to_follow;

	} else if (!port_stats_completed && msg->xid == port_stats_xid) {

		// continuation message
		for (const auto& it : msg->port_stats) {
			port_stats_shadow.push_back(it);
		}
		port_stats_completed = !msg->more_to_follow;
	}

	if (port_stats_completed) {

		if (port_stats_initial.empty()) {
			port_stats_initial = port_stats_shadow;
		}
		
		for (auto& shadow : port_stats_shadow) {
			for (const auto& initial : port_stats_initial) {
				if (initial.port == shadow.port) {
					shadow.rx_packets -= initial.rx_packets;
					shadow.tx_packets -= initial.tx_packets;
					shadow.rx_bytes -= initial.rx_bytes;
					shadow.tx_bytes -= initial.tx_bytes;
					shadow.rx_dropped -= initial.rx_dropped;
					shadow.tx_dropped -= initial.tx_dropped;
					shadow.rx_errors -= initial.rx_errors;
					shadow.tx_errors -= initial.tx_errors;
					shadow.rx_frame_errors -= initial.rx_frame_errors;
					shadow.rx_overrun_errors -= initial.rx_overrun_errors;
					shadow.rx_crc_errors -= initial.rx_crc_errors;
					shadow.collision_errors -= initial.collision_errors;
					break;
				}
			}
		}

		port_stats = port_stats_shadow;
	}
}

// updates queue stats from the switch
void operational_stats::update_handler(const shared_ptr<of_message_stats_reply_queue_stats>& msg) {
	lock_guard<mutex> g(lock);

	if (queue_stats_completed || msg->xid > queue_stats_xid) {

		// reset table and put in fresh contents
		queue_stats_shadow = msg->queue_stats;
		queue_stats_xid = msg->xid;
		queue_stats_completed = !msg->more_to_follow;

	} else if (!queue_stats_completed && msg->xid == queue_stats_xid) {

		// continuation message
		for (const auto& it : msg->queue_stats) {
			queue_stats_shadow.push_back(it);
		}
		queue_stats_completed = !msg->more_to_follow;
	}

	if (queue_stats_completed) {
		queue_stats = queue_stats_shadow;
	}
}

// retrieves the aggregate stats
openflow_aggregate_stats operational_stats::get_aggregate_stats() const {
	lock_guard<mutex> g(lock);
	return aggregate_stats;
}

// retrieves table stats
vector<openflow_table_stats> operational_stats::get_table_stats() const {
	lock_guard<mutex> g(lock);
	return table_stats;
}

// retrieves flows and stats
vector<openflow_flow_description_and_stats> operational_stats::get_flow_stats() const {
	lock_guard<mutex> g(lock);
	return flow_stats;
}

// retrieves port stats
vector<openflow_port_stats> operational_stats::get_port_stats() const {
	lock_guard<mutex> g(lock);
	return port_stats;
}

// retrieves queue stats
vector<openflow_queue_stats> operational_stats::get_queue_stats() const {
	lock_guard<mutex> g(lock);
	return queue_stats;
}

// clears all objects (but does not affect the shadow entries)
void operational_stats::clear() {
	lock_guard<mutex> g(lock);
	aggregate_stats.clear();
	flow_stats.clear();
	table_stats.clear();
	port_stats.clear();
	queue_stats.clear();
}

// return information about this service
string operational_stats::get_service_info() const {
	char buf[128];
	sprintf(buf, "openflow operational stats service version %d.%d", service_major_version, service_minor_version);
	return string(buf);
}

// return running information on this service
string operational_stats::get_running_info() const {
	return string();
}
