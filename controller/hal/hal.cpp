#include "hal.h"
#include "../../common/timer.h"
#include "../openflow_messages/of_message_factory.h"
#include "../services/arp.h"
#include "../services/cam.h"
#include "../services/flow_service.h"
#include "../services/flow_policy_checker.h"
#include "../services/flow_table.h"
#include "../services/dell_s48xx_l2_table.h"
#include "../services/dell_s48xx_acl_table.h"
#include "../services/ironstack_echo_daemon.h"
#include "../services/operational_stats.h"
#include "../services/switch_state.h"
#include "../gui/output.h"

// expose some statistics here for the GUI
uint64_t controller_bytes_sent = 0;
uint64_t controller_bytes_received = 0;

// constructor
hal::hal() {
	atomic_store(&switch_ready, false);
	atomic_store(&under_initialization, false);
	atomic_store(&shutdown_flag, false);
	svc_catalog.set_controller(this);
	svc_catalog.clear();
	switch_response_time_ms = -1;
}

// destructor calls shutdown to reset all state and close threads
hal::~hal() {
	shutdown();
}

// initializes the hardware; blocks until a connection is made from the switch
bool hal::init(uint16_t port, const set<shared_ptr<service>>& services,
	const set<ip_address>& allowed_ip_addresses) {

	bool expected = false;
	if (!under_initialization.compare_exchange_strong(expected, true)) {
		output::log(output::loglevel::VERBOSE, "hal::init() -- already under initialization.\n");
		return false;
	}

	// check if the switch is ready or currently in the process of being shutdown
	// note that switch_ready and shutdown_flag cannot both be true (the shutdown
	// function toggles the switch_ready flag first)
	if (atomic_load(&switch_ready) && !atomic_load(&shutdown_flag)) {
		output::log(output::loglevel::VERBOSE, "hal::init() -- already initialized.\n");
		return true;
	}

	// reset the switch response time
	switch_response_time_ms = -1;

	// setup socket for listening
	output::log("ironstack openflow controller.\n");
	if (!tcp::listen(port)) {
		output::log(output::loglevel::ERROR, "hal::init() -- unable to listen on port %hu.\n", port);
		return false;
	}	else {
		output::log(output::loglevel::INFO, "listening for openflow connection on port %d.\n", port);
	}

	// wait and accept a connection
	tcp_info info;
	while(1) {
		if (connection.accept(port) && connection.get_connection_info().is_connected()) {
			info = connection.get_connection_info();

			// if all addresses are allowed, continue with handshaking
			if (allowed_ip_addresses.empty() ||
				allowed_ip_addresses.count(info.get_remote_ip_address()) > 0) {
				break;
			} else {
				output::log(output::loglevel::VERBOSE, "openflow connection rejected. ov-switch:[%s:%u] not in allowed list.\n",
					info.get_remote_ip_address().to_string().c_str(),
					info.get_remote_port());
			}
		}
		connection.close();
	}

	// with the openflow connection made, stop listening so other processes can listen
	tcp::stop_listen(port);

	// connected to a switch on the allowed ip address list
	output::log(output::loglevel::INFO, "openflow connection established. ov-switch:[%s:%u] local-endpoint:[%s:%u].\n",
		info.get_remote_ip_address().to_string().c_str(),
		info.get_remote_port(),
		info.get_local_ip_address().to_string().c_str(),
		info.get_local_port());

	// setup common variables
	shared_ptr<of_message> received_msg;
	autobuf serialized_msg;
	autobuf socket_buf;

	// send hello
	output::log(output::loglevel::INFO, "openflow handshaking with switch begins.\n");
	shared_ptr<of_message_hello> m_hello(new of_message_hello());
	m_hello->xid = hal_transaction::reserve_xid();
	m_hello->serialize(serialized_msg);

	if (connection.send_raw(serialized_msg)) {
		while(1) {
			received_msg = ironstack::net_utils::get_next_openflow_message(connection, socket_buf);
			if (received_msg == nullptr) {
				output::log(output::loglevel::ERROR, "openflow handshaking failed.\n");
				connection.close();
				return false;
			} else if (received_msg->msg_type == OFPT_HELLO) {
				output::log(output::loglevel::INFO, "the openflow handshake completed successfully.\n");
				break;
			} else {
				asynchronous_messages.enqueue(received_msg);
				output::log(output::loglevel::INFO, "  enqueued message type [%s] for deferred processing.\n",
					received_msg->get_message_type_string().c_str());
			}
		}
	} else {
		output::log(output::loglevel::ERROR, "hal::init() -- handshaking failed.\n");
		connection.close();
		return false;
	}

	// set fragmentation/packet-in miss len
	output::log(output::loglevel::INFO, "setting switch fragmentation [normal] packet-in size [65535].\n");
	shared_ptr<of_message_set_config> m_set_config(new of_message_set_config());
	m_set_config->xid = hal_transaction::reserve_xid();
	m_set_config->frag_normal = true;
	m_set_config->max_msg_send_len = 65535;
	m_set_config->serialize(serialized_msg);

	if (connection.send_raw(serialized_msg)) {
		output::log(output::loglevel::INFO, "switch parameters submitted (no completion guarantee!).\n");
	} else {
		output::log(output::loglevel::ERROR, "failed to set switch parameters.\n");
		connection.close();
		return false;
	}

	// send echo request
	output::log(output::loglevel::INFO, "sending openflow echo request.\n");
	shared_ptr<of_message_echo_request> m_echo_req(new of_message_echo_request());
	m_echo_req->xid = hal_transaction::reserve_xid();
	m_echo_req->serialize(serialized_msg);

	if (connection.send_raw(serialized_msg)) {
		while(1) {
			received_msg = ironstack::net_utils::get_next_openflow_message(connection, socket_buf);
			if (received_msg == nullptr) {
				output::log(output::loglevel::ERROR, "openflow echo request failed.\n");
				connection.close();
				return false;
			} else if (received_msg->msg_type == OFPT_ECHO_REPLY) {
				output::log(output::loglevel::INFO, "openflow echo reply received successfully.\n");
				break;
			} else {
				asynchronous_messages.enqueue(received_msg);
				output::log(output::loglevel::INFO, "  enqueued message type [%s] for deferred processing.\n",
					received_msg->get_message_type_string().c_str());
			}
		}
	} else {
		output::log(output::loglevel::ERROR, "hal::init() -- echo request failed.\n");
		connection.close();
		return false;
	}

	output::log(output::loglevel::INFO, "hal::init() openflow stage 1 initialization completed.\n");
	
	// flush all services in the catalog and install hardware specific drivers
	output::log(output::loglevel::INFO, "registering ironstack service modules.\n");
	svc_catalog.clear();
	for (const auto& s : services) {
		svc_catalog.register_service(s);
	}

	// setup controller readiness flags so that threads and services can begin to
	// use controller functions
	atomic_store(&switch_ready, true);
	atomic_store(&shutdown_flag, false);
	atomic_store(&under_initialization, false);

	// startup auxiliary threads
	output::log(output::loglevel::INFO, "hal now starting processing threads.\n");
	recv_loop_thread = thread(&hal::recv_loop_entrypoint, this);
	send_loop_thread = thread(&hal::send_loop_entrypoint, this);
	process_loop_thread = thread(&hal::process_message_loop_entrypoint, this);
	maintenance_thread = thread(&hal::maintenance_thread_entrypoint, this);

	// call init2() on all services
	svc_catalog.deferred_init_services();

	// start up packet in processor
	packet_processor.init(1);

	// all done
	output::log(output::loglevel::INFO, "ironstack controller started.\n");
	return true;
}

// prepares the controller for shutdown
void hal::shutdown() {

	// no need to do anything else if the shutdown is already complete/in progress
	bool expected = true;
	if (!switch_ready.compare_exchange_strong(expected, false)) {
		output::log(output::loglevel::WARNING, "hal::shutdown() -- switch not initialized.\n");
		return;
	}

	expected = false;
	if (!shutdown_flag.compare_exchange_strong(expected, true)) {
		output::log(output::loglevel::WARNING, "hal::shutdown() -- shutdown() already called.\n");
		return;
	}

	// set switch status to 'not ready'
	atomic_store(&switch_ready, false);
	output::log(output::loglevel::INFO, "hal::shutdown() -- the controller is now shutting down.\n");

	pending_transactions.enqueue(nullptr);
	asynchronous_messages.enqueue(nullptr);

	// shutdown the tcp connection
	connection.close();

	output::log(output::loglevel::INFO, "hal::shutdown() -- joining threads now.\n");

	output::log(output::loglevel::INFO, "joining send thread.\n");
	send_loop_thread.join();
	output::log(output::loglevel::INFO, "joining receive thread.\n");
	recv_loop_thread.join();
	output::log(output::loglevel::INFO, "joining process loop thread.\n");
	process_loop_thread.join();
	output::log(output::loglevel::INFO, "joining maintenance thread.\n");
	maintenance_thread.join();
	output::log(output::loglevel::INFO, "hal::shutdown() complete.\n");

	// clear all data structures

	// TODO -- inform all transactions that they have failed
	callback_transactions.clear();
	packet_processor.clear();
	packet_processor.shutdown();

	// TODO -- inform all pending transactions that they have failed
	pending_transactions.clear();
	asynchronous_messages.clear();

	atomic_store(&switch_ready, false);
	atomic_store(&shutdown_flag, false);
}

// sends an echo request to the switch
bool hal::send_echo_request(int* response_time_ms) {

	timer response_time_timer;
	shared_ptr<of_message_echo_request> request(new of_message_echo_request());
	shared_ptr<blocking_hal_callback> hal_echo_request_callback(new blocking_hal_callback());
	shared_ptr<hal_transaction> transaction(new hal_transaction(request, true, hal_echo_request_callback));
	pending_transactions.enqueue(transaction);

	// block while waiting for a response
	// if the response is a false, that means the transaction failed
	if (!hal_echo_request_callback->wait() || !hal_echo_request_callback->is_transaction_successful()) {
		if (response_time_ms != nullptr) {
			*response_time_ms = -1;
		}
		return false;
	}

	// success, return the timing if requested
	if (response_time_ms != nullptr) {
		*response_time_ms = response_time_timer.get_time_elapsed_ms();
	}
	return true;
}

// sends a barrier request to the switch
bool hal::send_barrier_request(int* response_time_ms) {

	timer response_time_timer;
	shared_ptr<of_message_barrier_request> request(new of_message_barrier_request());
	shared_ptr<blocking_hal_callback> hal_barrier_request_callback(new blocking_hal_callback());
	shared_ptr<hal_transaction> transaction(new hal_transaction(request, true, hal_barrier_request_callback));
	pending_transactions.enqueue(transaction);

	// block until barrier returns and is successful
	if (!hal_barrier_request_callback->wait() || !hal_barrier_request_callback->is_transaction_successful()) {
		if (response_time_ms != nullptr) {
			*response_time_ms = -1;
		}
		return false;
	}

	// update the switch response time
	switch_response_time_ms = response_time_timer.get_time_elapsed_ms();	

	// return the response time if required
	if (response_time_ms != nullptr) {
		*response_time_ms = switch_response_time_ms;
	}
	return true; 
}

// returns the response time of the switch (to the last barrier request)
int hal::get_switch_response_time() {
	return switch_response_time_ms;
}

// sends a packet to a set of physical ports
bool hal::send_packet(const autobuf& packet, const set<uint16_t>& phy_ports) {
	
	if (!atomic_load(&switch_ready)) {
		output::log(output::loglevel::ERROR, "hal::send_packet() -- switch is not in the ready mode.\n");
		return false;
	}

	// generate the packet out request
	shared_ptr<of_message_packet_out> pkt_out(new of_message_packet_out());
	pkt_out->packet_data = packet.copy_as_read_only();

	// generate an output action for each physical port
	for (const auto& it : phy_ports) {
		of_action_output_to_port action;
		action.port = it;
		pkt_out->action_list.add_action(action);
	}

	shared_ptr<hal_transaction> transaction(new hal_transaction(pkt_out, false));
	pending_transactions.enqueue(transaction);

	return true;
}

// sends a packet out a single physical port
bool hal::send_packet(const autobuf& packet, uint16_t phy_port) {

	set<uint16_t> port_set = { phy_port };
	return send_packet(packet, port_set);
}

// sends a packet to a given IP address
bool hal::send_packet(const autobuf& packet, const ip_address& nw_addr) {
	return false;
}

// sends a packet to a given MAC address
bool hal::send_packet(const autobuf& packet, const mac_address& dl_addr) {
	return false;
}

// returns a reference to the internal service catalog
service_catalog* hal::get_service_catalog() {
	return &svc_catalog;
}

// gets a pointer to a given ironstack service type 
shared_ptr<service> hal::get_service(service_catalog::service_type svc_type) {
	return svc_catalog.get_service(svc_type);
}

// gets a pointer to the packet processing facility
packet_in_processor* hal::get_packet_processor() {
	return &packet_processor;
}

// queues a hal transaction for processing
void hal::enqueue_transaction(const shared_ptr<hal_transaction>& transaction) {
	pending_transactions.enqueue(transaction);
}

// this thread loop handles the sending of packets to the switch
void hal::send_loop_entrypoint() {

	shared_ptr<hal_transaction> current_msg;
	while(!atomic_load(&shutdown_flag)) {
		
		// get current message
		current_msg = pending_transactions.dequeue();
		if (current_msg == nullptr) continue;

		// if the message requires callbacks, put it into the callback queue before
		// sending out of the socket, or there will be a race condition and callbacks
		// could be lost
		if (current_msg->has_completion_acknowledgement()) {
			lock_guard<mutex> g(callback_transactions_lock);
			callback_transactions.push_back(current_msg);
		}

		// send message to the switch
		if (!connection.send_raw(*current_msg->get_serialized_msg())) {
			output::log(output::loglevel::ERROR, "hal::send_loop_entrypoint() -- could not send the openflow message.\n");
			if (!connection.get_connection_info().is_connected()) {
				output::log(output::loglevel::ERROR, "hal::send_loop_entrypoint() -- controller connection broken.\n");
			}
			break;
		}
		controller_bytes_sent = connection.get_connection_info().get_bytes_sent();
	}

	output::log(output::loglevel::INFO, "hal send loop shutdown completed.\n");
}

// this thread loop handles packets received from the switch
void hal::recv_loop_entrypoint() {

	shared_ptr<of_message> current_msg;
	autobuf sock_buf;
	while(!atomic_load(&shutdown_flag)) {

		// read the next message from the connection
		current_msg = ironstack::net_utils::get_next_openflow_message(connection, sock_buf);
		if (current_msg == nullptr) {
			output::log(output::loglevel::ERROR, "hal::recv_loop_entrypoint() -- could not read the next openflow message.\n");
			if (!connection.get_connection_info().is_connected()) {
				output::log(output::loglevel::ERROR, "hal::recv_loop_entrypoint() --- controller connection broken.\n");
			}
			break;
		}
		controller_bytes_received = connection.get_connection_info().get_bytes_received();

		// enqueue for processing
		asynchronous_messages.enqueue(current_msg);
	}

	output::log(output::loglevel::INFO, "hal recv loop shutdown completed.\n");
}

// this thread handles packets that are read in from the switch
void hal::process_message_loop_entrypoint() {

	bool perform_callbacks;  // set to false if no callback processing required
	bool callback_status;    // set to the result of the callback, if any
	shared_ptr<of_message> current_msg;
	autobuf serialized_reply;

	while (!atomic_load(&shutdown_flag)) {

		perform_callbacks = false;
		callback_status = false;

		// get the next message
		current_msg = asynchronous_messages.dequeue();
		if (current_msg == nullptr) continue;

		// process msg here
		switch (current_msg->msg_type) {

			// error message
			case (OFPT_ERROR):
			{
				shared_ptr<of_message_error> msg = static_pointer_cast<of_message_error>(current_msg);
				output::log(output::loglevel::ERROR, "\n*** the openflow switch has sent an error ***\nmsg xid: %d\nerror message contents:\n%s\n\n", msg->xid, msg->to_string().c_str());

				// log the error message into a file
				FILE* fp = fopen("error.txt", "a");
				if (fp != nullptr) {
					fprintf(fp, "An OpenFlow error has occurred. This may be a bug in the ironstack code.\n\nmsg xid: %d\n", msg->xid);
					fprintf(fp, "verbose error: \n%s\n\n", msg->to_string().c_str());
					fprintf(fp, "error data hex output:\n%s\n", msg->error_data.to_hex().c_str());
					fclose(fp);
				}

				perform_callbacks = true;
				callback_status = false;
				break;
			}

			// echo and barrier replies
			case (OFPT_ECHO_REPLY):
			case (OFPT_BARRIER_REPLY):
			{
				perform_callbacks = true;
				callback_status = true;
				break;
			}

			// switch wants an echo reply
			case (OFPT_ECHO_REQUEST):
			{
				shared_ptr<of_message_echo_request> msg = static_pointer_cast<of_message_echo_request>(current_msg);
				shared_ptr<of_message_echo_reply> echo_reply(new of_message_echo_reply());
				echo_reply->xid = msg->xid;
				echo_reply->data = msg->data;
				shared_ptr<hal_transaction> transaction(new hal_transaction());
				transaction->set_request_and_serialize(echo_reply, false);
				pending_transactions.enqueue(transaction);

				perform_callbacks = false;
				break;
			}

			// raw packet input -- enqueue to packet processor
			case (OFPT_PACKET_IN):
			{
				shared_ptr<of_message_packet_in> msg = static_pointer_cast<of_message_packet_in>(current_msg);
				packet_processor.enqueue_packet(msg);

				perform_callbacks = false;
				break;
			}

			// switch stats reply
			case (OFPT_STATS_REPLY):
			{
				shared_ptr<of_message_stats_reply> base_msg = static_pointer_cast<of_message_stats_reply>(current_msg);
				shared_ptr<operational_stats> op_stats = static_pointer_cast<operational_stats>(get_service(service_catalog::service_type::OPERATIONAL_STATS));
				switch (base_msg->stats_type) {
	
					// flow statistics update. inform flow service and operational stats
					case of_message_stats_reply::stats_t::FLOW_STATS:
					{
						// cast into proper message subtype
						shared_ptr<of_message_stats_reply_flow_stats> flow_stats_msg = static_pointer_cast<of_message_stats_reply_flow_stats>(current_msg);

						// flow service update
						shared_ptr<flow_service> flow_svc = static_pointer_cast<flow_service>(get_service(service_catalog::service_type::FLOWS));
						if (flow_svc != nullptr) {
							flow_svc->flow_update_handler(*flow_stats_msg);
						} else {
							output::log(output::loglevel::WARNING, "hal message callbacks: flow service offline. cannot deliver flow stats reply.\n");
						}

						// operational stats update
						if (op_stats != nullptr) {
							op_stats->update_handler(flow_stats_msg);
						} else {
							output::log(output::loglevel::WARNING, "hal message callbacks: operational stats service offline. no realtime stats update.\n");
						}
						break;
					}

					// aggregate statistics of all flows (counts packets, flows and bytes)
					case of_message_stats_reply::stats_t::AGGREGATE_STATS:
					{
						shared_ptr<of_message_stats_reply_aggregate_stats> aggregate_stats_msg = static_pointer_cast<of_message_stats_reply_aggregate_stats>(current_msg);
						if (op_stats != nullptr) {
							op_stats->update_handler(aggregate_stats_msg);
						} else {
							output::log(output::loglevel::WARNING, "hal message callbacks: operational stats service offline. no realtime stats update.\n");
						}
						break;
					}

					// table statistics update (gets table identifier, wildcard match type, capacity, active flows, matched count, etc)
					case of_message_stats_reply::stats_t::TABLE_STATS:
					{
						shared_ptr<of_message_stats_reply_table_stats> table_stats_msg = static_pointer_cast<of_message_stats_reply_table_stats>(current_msg);
						if (op_stats != nullptr) {
							op_stats->update_handler(table_stats_msg);
						} else {
							output::log(output::loglevel::WARNING, "hal message callbacks: operational stats service offline. no realtime stats update.\n");
						}
						break;
					}

					// port statistics update. counts packets, bytes and errors.
					case of_message_stats_reply::stats_t::PORT_STATS:
					{
						shared_ptr<of_message_stats_reply_port_stats> port_stats_msg = static_pointer_cast<of_message_stats_reply_port_stats>(current_msg);
						if (op_stats != nullptr) {
							op_stats->update_handler(port_stats_msg);
						} else {
							output::log(output::loglevel::WARNING, "hal message callbacks: operational stats service offline. no realtime stats update.\n");
						}
						break;
					}

					// queue statistics update. counts packets for a given queue id
					case of_message_stats_reply::stats_t::QUEUE_STATS:
					{
						shared_ptr<of_message_stats_reply_queue_stats> queue_stats_msg = static_pointer_cast<of_message_stats_reply_queue_stats>(current_msg);
						if (op_stats != nullptr) {
							op_stats->update_handler(queue_stats_msg);
						} else {
							output::log(output::loglevel::WARNING, "hal message callbacks: operational stats service offline. no realtime stats update.\n");
						}
						break;
					}
					
					// switch description. updated into switch state.
					case of_message_stats_reply::stats_t::SWITCH_DESCRIPTION: {
						shared_ptr<of_message_stats_reply_switch_description> sw_desc_msg = static_pointer_cast<of_message_stats_reply_switch_description>(current_msg);
						shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(get_service(service_catalog::service_type::SWITCH_STATE));
						if (sw_state != nullptr) {
							sw_state->set_switch_description(*sw_desc_msg);
						}
					}
					default:
						break;
				}

				perform_callbacks = true;
				callback_status = true;
				break;
			}

			// switch features reply
			case (OFPT_FEATURES_REPLY):
			{
				shared_ptr<of_message_features_reply> msg = static_pointer_cast<of_message_features_reply>(current_msg);
				shared_ptr<switch_state> switch_state_service = static_pointer_cast<switch_state>(get_service(service_catalog::service_type::SWITCH_STATE));
				if (switch_state_service != nullptr) {
					switch_state_service->set_switch_features(*msg);
				}

				perform_callbacks = false;
				break;
			}

			// switch configuration reply
			case (OFPT_GET_CONFIG_REPLY):
			{
				shared_ptr<of_message_get_config_reply> msg = static_pointer_cast<of_message_get_config_reply>(current_msg);
				shared_ptr<switch_state> switch_state_service = static_pointer_cast<switch_state>(get_service(service_catalog::service_type::SWITCH_STATE));
				if (switch_state_service != nullptr) {
					switch_state_service->set_switch_config(*msg);
				}

				perform_callbacks = false;
				break;
			}

			// flow removed message
			case (OFPT_FLOW_REMOVED):
			{
				shared_ptr<of_message_flow_removed> msg = static_pointer_cast<of_message_flow_removed>(current_msg);
				shared_ptr<flow_service> flow_svc = static_pointer_cast<flow_service>(get_service(service_catalog::service_type::FLOWS));
				if (flow_svc != nullptr) {
					flow_svc->flow_update_handler(*msg);
				} else {
					output::log(output::loglevel::WARNING, "hal message callback: flow service offline. cannot deliver flow removed message.\n");
				}

				perform_callbacks = false;
				break;
			}

			// port status has changed
			case (OFPT_PORT_STATUS):
			{
				shared_ptr<of_message_port_status> msg = static_pointer_cast<of_message_port_status>(current_msg);
				shared_ptr<switch_state> switch_state_service = static_pointer_cast<switch_state>(get_service(service_catalog::service_type::SWITCH_STATE));
				if (switch_state_service != nullptr) {
					switch_state_service->update_switch_port(*msg);
				}
				perform_callbacks = false;
				break;
			}

			// should never get here (implies that we did not recognize some openflow message)
			default:
			{
				output::log(output::loglevel::BUG, "hal: unprocessed message received [xid %d]. contents:\n%s\n\n", current_msg->xid, current_msg->to_string().c_str());
				perform_callbacks = false;
				break;
			}
		}

		// process callbacks if required
		if (perform_callbacks) {
			uint32_t xid = current_msg->xid;
		
			// if this is a barrier reply, handle all preceding callbacks as successful (since no error messages were generated)
			if (current_msg->msg_type == OFPT_BARRIER_REPLY) {
				lock_guard<mutex> g(callback_transactions_lock);
				auto iterator = callback_transactions.begin();
				while (iterator != callback_transactions.end()) {

					uint32_t current_xid = (*iterator)->get_request()->xid;
					if (current_xid <= xid) {
						if (current_xid == xid) {
							// barrier message should take the barrier reply
							(*iterator)->set_reply(current_msg);
							(*iterator)->get_callback()->hal_callback((*iterator), current_msg, true);
						} else if (current_xid <= xid) {
							// other messages have no replies (ie, success)
							(*iterator)->set_reply(nullptr);
							if ((*iterator)->get_callback() != nullptr) {
								(*iterator)->get_callback()->hal_callback((*iterator), nullptr, true);
							} else {
								output::log(output::loglevel::BUG, "hal callback -- null callback.\n");
							}
						}
						// if callback was fired, remove it from the callback backlog
						iterator = callback_transactions.erase(iterator);
					} else {
						++iterator;
					}
				}

			// if this wasn't a barrier reply, handle only the callback to this transaction
			} else {
				lock_guard<mutex> g(callback_transactions_lock);
				auto iterator = callback_transactions.begin();
				while (iterator != callback_transactions.end()) {
					if ((*iterator)->get_request()->xid == xid) {
						(*iterator)->set_reply(current_msg);
						(*iterator)->get_callback()->hal_callback((*iterator), current_msg, callback_status);
						
						callback_transactions.erase(iterator);
						break;
					} else {
						++iterator;
					}
				}
			}
		}

	}

	output::log(output::loglevel::INFO, "hal process message loop shutdown completed.\n");
}

// this thread periodically polls the callback list and decides if a barrier should be
// posted to unblock all pending messages
void hal::maintenance_thread_entrypoint() {

	bool should_post_barrier = false;
	while (!atomic_load(&shutdown_flag)) {
		
		{
			lock_guard<mutex> g(callback_transactions_lock);
			should_post_barrier = (!callback_transactions.empty());
		}

		if (should_post_barrier) {
			send_barrier_request();	// TODO should not be blocking, needs to check shutdown flag as well
		}

		timer::sleep_for_ms(1000);
	}

	output::log(output::loglevel::INFO, "hal maintenance thread loop shutdown completed.\n");
}
