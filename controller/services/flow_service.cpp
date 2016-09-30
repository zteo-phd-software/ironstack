#include "arp.h"
#include "cam.h"
#include "flow_service.h"
#include "inter_ironstack_service.h"
#include "../gui/output.h"

// initializes the flow service
// init is called after all the flow tables have been attached
bool flow_service::init() {

	{
		lock_guard<mutex> g(lock);
		if (initialized || initializing) {
			output::log(output::loglevel::WARNING, "flow_service::init() -- cannot be re-initialized.\n");
			return true;
		}

		initializing = true;
	}

	// can't start without flow tables
	if (flow_tables.empty()) {
		output::log(output::loglevel::BUG, "flow_service::init() -- cannot start flow service without flow tables.\n");
		assert(0);
		abort();
	}
	
	return true;
}

// called after the controller has been initialized
bool flow_service::init2() {

	// register a callback for port modification events on all vlans
	shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
	if (sw_state == nullptr) {
		output::log(output::loglevel::WARNING, "flow_service::init2() -- switch state offline; can't register port modification callbacks.\n");
	} else {

		port_cob = make_shared<flow_service_port_mod_callback>();
		port_cob->init(controller);
		set<uint16_t> openflow_vlans = sw_state->get_vlan_ids();
		for (const auto& vlan : openflow_vlans) {
			sw_state->register_port_modification_callback(port_cob, vlan);
		}

		output::log(output::loglevel::INFO, "flow_service::init2() -- registered port modification callback.\n");
	}

	// download all flows as a first step
	shared_ptr<of_message_stats_request_flow_stats> request(new of_message_stats_request_flow_stats());
	request->fields_to_match.wildcard_all();
	request->all_tables = true;
	request->restrict_out_port = false;
	shared_ptr<blocking_hal_callback> hal_request_callback(new blocking_hal_callback());
	shared_ptr<hal_transaction> transaction(new hal_transaction(request, false, nullptr));
	controller->enqueue_transaction(transaction);

	// wait for flows to be received
	int response_time_ms;
	if (!controller->send_barrier_request(&response_time_ms)) {
		output::log(output::loglevel::ERROR, "flow_service::init2() could not initialize because flows could not be downloaded.\n");
		abort();
	} else {
		output::log(output::loglevel::INFO, "flow_service::init2() -- flows downloaded in %dms.\n", response_time_ms);
	}


	// service is ready
	{
		lock_guard<mutex> g(lock);
		initialized = true;
		initializing = false;
	}
	output::log(output::loglevel::INFO, "flow_service::init2() service initialized.\n");

	return true;
}

// shuts down the flow service (does not detach flow tables or reset cookie counter)
void flow_service::shutdown() {

	{
		lock_guard<mutex> g(lock);

		// nothing to cleanup if not initialized to begin with
		if (!initialized && !initializing) {
			return;
		}
	}

	// if the system is being initialized, spin until it is done
	while (1) {
		lock.lock();
		if (!initialized && initializing) {
			lock.unlock();
			timer::sleep_for_ms(1000);
		} else {
			break;
		}
	}

	last_stats_update_xid = 0;
	initialized = false;
	lock.unlock();
}

// attach flow table
// can only be done before the system is initialized
void flow_service::attach_flow_table(const shared_ptr<flow_table>& table) {

	lock_guard<mutex> g(lock);
	if (!initialized) {
		flow_tables.push_back(table);
	} else {
		output::log(output::loglevel::BUG, "flow_service::attach_flow_table() cannot attach flow table when service is live.\n");
	}
}

// detach a flow table
// can only be done before the system is initialized
void flow_service::detach_flow_table(const shared_ptr<flow_table>& table) {

	lock_guard<mutex> g(lock);
	if (!initialized) {
		auto iterator = flow_tables.begin();
		while (iterator != flow_tables.end()) {
			if (*iterator == table) {
				flow_tables.erase(iterator);
				break;
			} else {
				++iterator;
			}
		}
	} else {
		output::log(output::loglevel::BUG, "flow_service::detach_flow_table() cannot detach flow table when service is live.\n");
	}
}

// sends the controller a message to refresh all flows and wait until the request is complete
bool flow_service::refresh_flows(int timeout_ms) {

	{
		lock_guard<mutex> g(lock);
		if (!initialized) {
			output::log(output::loglevel::WARNING, "flow_service::refresh_flows() cannot refresh flows; service offline.\n");
			return false;
		}
	}

	shared_ptr<of_message_stats_request_flow_stats> request(new of_message_stats_request_flow_stats());
	request->fields_to_match.wildcard_all();
	request->all_tables = true;
	request->restrict_out_port = false;

	// setup for nonblocking or blocking callback
	shared_ptr<hal_transaction> transaction;
	shared_ptr<blocking_hal_callback> hal_barrier_request_callback;

	// timeouts of -1 or > 0 implies blocking wait
	if (timeout_ms != 0) {
		hal_barrier_request_callback = make_shared<blocking_hal_callback>();
		transaction = make_shared<hal_transaction>(request, true, hal_barrier_request_callback);
	} else if (timeout_ms == 0) {
		transaction = make_shared<hal_transaction>(request, false);
	}

	// send the request
	controller->enqueue_transaction(transaction);

	// wait if applicable
	bool status = true;
	if (timeout_ms < 0) {
		status = hal_barrier_request_callback->wait();
	} else if (timeout_ms > 0) {
		status = hal_barrier_request_callback->timed_wait(timeout_ms);
	}
	
	return status;
}

// clears out all flows on the switch
bool flow_service::clear_all_flows(int timeout_ms, bool keep_static_flows) {

	{
		lock_guard<mutex> g(lock);
		if (!initialized) {
			output::log(output::loglevel::WARNING, "flow_service::clear_all_flows() cannot clear flows; service offline.\n");
			return false;
		}
	}

	// if static flows have to be kept, then the removal process is more tedious
	// since each flow has to be removed independently
	shared_ptr<of_message_modify_flow> request;
	shared_ptr<hal_transaction> transaction;
	shared_ptr<blocking_hal_callback> hal_blocking_request;
	if (keep_static_flows) {

		// compile list of flows to remove
		vector<openflow_flow_entry> flows_to_remove;
		for (auto& table : flow_tables) {
			auto current_flows_to_remove = table->mark_all_flows_as_pending_delete(false);
			flows_to_remove.insert(flows_to_remove.end(), current_flows_to_remove.begin(), current_flows_to_remove.end());
		}

		// table entry has been marked so remove each flow individually
		uint32_t number_of_flows = flows_to_remove.size();
		if (number_of_flows == 0) {
			return true;
		}

		// iterate over each flow and remove all but the last flow independently
		for (uint32_t counter = 0; counter < number_of_flows; ++counter) {
			request = make_shared<of_message_modify_flow>();
			request->flow_description = flows_to_remove[counter].description;
			request->flow_description.cookie = flows_to_remove[counter].description.cookie;
			request->flow_description.action_list.clear();
			request->command = OFPFC_DELETE_STRICT;
			request->use_out_port = false;

			if (counter != number_of_flows-1) {
				transaction = make_shared<hal_transaction>(request, false);
				controller->enqueue_transaction(transaction);
			} else {
				break;
			}
		}

	// if all flows (including static ones) have to be removed, issue a generic
	// flow removal request to delete all flows
	} else {
	
		// mark all table entries as being deleted
		for (auto& table : flow_tables) {
			table->mark_all_flows_as_pending_delete(true);
		}

		// prepare the request
		request = make_shared<of_message_modify_flow>();
		request->flow_description.criteria.wildcard_all();
		request->flow_description.cookie = 0;
		request->command = OFPFC_DELETE;
		request->use_out_port = false;
	}

	// setup the timeout params
	if (timeout_ms != 0) {
		hal_blocking_request = make_shared<blocking_hal_callback>();
		transaction = make_shared<hal_transaction>(request, true, hal_blocking_request);
	} else {
		transaction = make_shared<hal_transaction>(request, false);
	}
	controller->enqueue_transaction(transaction);

	// wait for the confirmation (if needed)
	bool status = true;
	if (timeout_ms != 0) {
		if (timeout_ms < 0) {
			status = hal_blocking_request->wait();
		} else if (timeout_ms > 0) {
			status = hal_blocking_request->timed_wait(timeout_ms);
		}

		if (status) {
			for (auto& table : flow_tables) {
				table->clear_table();
			}
			output::log(output::loglevel::INFO, "flow_service:: all %sflows have been cleared from this switch.\n", keep_static_flows ? "non-static " : "");
		} else {
			output::log(output::loglevel::ERROR, "flow_service::clear_all_flows() could not clear flows; operation timed out.\n");
		}
	}

	return status;
}

// adds a flow using default priority and timeouts. automatic cookie selection
uint64_t flow_service::add_flow_auto(const of_match& criteria, const openflow_action_list& action_list, const string& reason, bool is_static, int timeout_ms) {
	return add_flow_auto(criteria, action_list, reason, default_priority, is_static, timeout_ms);
}

// adds a flow using a specified priority. default timeouts.
uint64_t flow_service::add_flow_auto(const of_match& criteria, const openflow_action_list& action_list, const string& reason, uint16_t priority, bool is_static, int timeout_ms) {
	openflow_flow_description description;
	description.criteria = criteria;
	description.action_list = action_list;
	description.priority = priority;
	description.cookie = flow_table::get_next_available_cookie_id();

	return add_flow(description, reason, default_idle_timeout, default_hard_timeout, is_static, timeout_ms);
}

// adds a fully specified flow with custom idle and hard timeout
uint64_t flow_service::add_flow(const openflow_flow_description& description, const string& reason, uint16_t idle_timeout, uint16_t hard_timeout, bool is_static, int install_timeout_ms) {

	{
		lock_guard<mutex> g(lock);

		if (!initialized) {
			output::log(output::loglevel::ERROR, "flow_service::add_flow() could not add flow because the service is offline.\n");
			return false;
		}
	}

	// synthesize the table entry
	openflow_flow_entry new_entry;
	new_entry.state = openflow_flow_entry::flow_state::PENDING_INSTALLATION;
	new_entry.description = description;
	new_entry.install_reason = reason;
	new_entry.idle_timeout = is_static ? 0 : idle_timeout;
	new_entry.hard_timeout = is_static ? 0 : hard_timeout;

	// find a table that will accept this entry
	for (auto& table : flow_tables) {
		if (table->check_table_fit(description)) {

//			output::log(output::loglevel::INFO, "flow_service::checking if flow %s exists.\n", description.to_string().c_str());

			// check if the flow is already alive and well
			uint64_t cookie = table->get_cookie_for_flow(description);
			openflow_flow_entry flow_entry;
			if (cookie != ((uint64_t) -1) 
				&& table->get_flow_entry_by_cookie(cookie, flow_entry)
				&& (flow_entry.state == openflow_flow_entry::flow_state::ACTIVE || flow_entry.state == openflow_flow_entry::flow_state::PENDING_INSTALLATION)) {
	//			output::log(output::loglevel::INFO, "flow_service::add_flow() -- flow already exists.\n");
				return cookie;
			}

			// flow doesn't exist. add it to the table
			if (!table->add_entry(new_entry)) {
				output::log(output::loglevel::ERROR, "flow_service::add_flow() could not add flow into table. flow capacity is reached.\n");
				return ((uint64_t) -1);
			}

			// construct the hal request
			output::log(output::loglevel::INFO, "flow_service::add_flow() adding flow:\n[%s]\n", description.to_string().c_str());
			shared_ptr<of_message_modify_flow> request(new of_message_modify_flow());
			request->flow_description = description;
			request->command = OFPFC_MODIFY_STRICT;
			request->idle_timeout = idle_timeout;
			request->hard_timeout = hard_timeout;
			request->buffer_id = -1;
			request->use_out_port = false;
			request->flag_send_flow_removal_message = true;
			request->flag_check_overlap = true;
			request->flag_emergency_flow = false;

			// setup for timed callback params
			shared_ptr<hal_transaction> transaction;
			shared_ptr<blocking_hal_callback> hal_blocking_request;

			if (install_timeout_ms != 0) {
				hal_blocking_request = make_shared<blocking_hal_callback>();
				transaction = make_shared<hal_transaction>(request, true, hal_blocking_request);
			} else {
				transaction = make_shared<hal_transaction>(request, false);
			}

			// enqueue and then wait for results
			bool status = true;
			controller->enqueue_transaction(transaction);

			// if no time-out, return the provisional cookie ID
			if (install_timeout_ms == 0) {
				return description.cookie;
			} else {

				// perform wait
				if (install_timeout_ms > 0) {
					status = hal_blocking_request->timed_wait(install_timeout_ms);
				} else {
					status = hal_blocking_request->wait();
				}

				// do callbacks
				table->hal_callback(transaction, nullptr, status);
				return description.cookie;
			}
		}
	}

	output::log(output::loglevel::BUG, "flow_service::add_flow() flow does not fit into any table. flow description:\n%s\n", description.to_string().c_str());
	return ((uint64_t) -1);
}

// removes a flow from the flow table. strict matching criteria
uint64_t flow_service::remove_flow_strict(const openflow_flow_description& flow) {
	return remove_flow(flow.cookie);
}

// removes a flow by its cookie ID
uint64_t flow_service::remove_flow(uint64_t cookie_id) {

	{
		lock_guard<mutex> g(lock);

		if (!initialized) {
			output::log(output::loglevel::ERROR, "flow_service::remove_flow() could not remove flow because the service is offline.\n");
			return (uint64_t) -1;
		}
	}

	// locate the table with this request
	openflow_flow_entry flow_entry;
	for (auto& table : flow_tables) {

		if (table->get_flow_entry_by_cookie(cookie_id, flow_entry)
			&& table->mark_entry_as_pending_deletion(cookie_id)) {

			// enqueue request
			shared_ptr<of_message_modify_flow> request(new of_message_modify_flow());
			request->flow_description = flow_entry.description;
			request->command = OFPFC_DELETE_STRICT;
			request->use_out_port = false;

			shared_ptr<hal_transaction> transaction(new hal_transaction(request, true, table));
			controller->enqueue_transaction(transaction);

			return cookie_id;
		}
	}

	return (uint64_t) -1;
}

// checks if a flow already exists in one of the flow tables. lookup by cookie id
uint64_t flow_service::does_flow_exist(uint64_t cookie_id) {

	{
		lock_guard<mutex> g(lock);

		if (!initialized) {
			output::log(output::loglevel::ERROR, "flow_service::does_flow_exist() could not check because the service is offline.\n");
			return (uint64_t) -1;
		}
	}

	for (const auto& flow_table : flow_tables) {
		if (flow_table->is_cookie_used(cookie_id)) {
			return cookie_id;
		}
	}

	return (uint64_t) -1;
}

// checks if a flow exists. lookup by flow description
uint64_t flow_service::does_flow_exist_strict(const openflow_flow_description& flow) const {

	{
		lock_guard<mutex> g(lock);

		if (!initialized) {
			output::log(output::loglevel::ERROR, "flow_service::does_flow_exist_strict() could not check because the service is offline.\n");
			return (uint64_t) -1;
		}
	}

	uint64_t result = (uint64_t) -1;
	for (const auto& flow_table : flow_tables) {
		if ((result = flow_table->get_cookie_for_flow(flow)) != ((uint64_t)-1)) {
			return result;
		}
	}
	return result;
}

// compile a list of flows from all flow tables
vector<openflow_flow_entry> flow_service::get_flows() const {

	vector<openflow_flow_entry> results;
	{
		lock_guard<mutex> g(lock);

		if (!initialized) {
			output::log(output::loglevel::ERROR, "flow_service::get_flows() could not get the list of flows because the service is offline.\n");
			return results;
		}
	}

	// get actual list of flows
	for (auto& table : flow_tables) {
		vector<openflow_flow_entry> table_flows = table->get_all_flows();
		results.insert(results.end(), table_flows.begin(), table_flows.end());
	}

	return results;
}

// returns a map of flows from all flow tables
map<uint64_t, openflow_flow_entry> flow_service::get_flows_map() const {
	
	map<uint64_t, openflow_flow_entry> results;
	{
		lock_guard<mutex> g(lock);
		if (!initialized) {
			output::log(output::loglevel::ERROR, "flow_service::get_flows_map() could not get the list of flows because the service is offline.\n");
			return results;
		}
	}

	// get map of flows
	for (auto& table : flow_tables) {
		auto flow_map = table->get_all_flows_map();
		results.insert(flow_map.begin(), flow_map.end());
	}

	return results;
}

// refreshes the flow table when flow stats are received
void flow_service::flow_update_handler(const of_message_stats_reply_flow_stats& stats) {

	// handle continuation messages
	pending_flows.insert(pending_flows.end(), stats.flow_stats.begin(), stats.flow_stats.end());
	if (stats.more_to_follow) return;

	// initialize the cookie id counter exactly once
	// flush the tables at the beginning of this init
	if (!initialized && initializing) {

		// compute the next available cookie ID
		uint64_t next_free_id = 1;
		for (const auto& flow : pending_flows) {
			if (flow.flow_description.cookie >= next_free_id) {
				next_free_id = flow.flow_description.cookie+1;
			}
		}
		flow_table::set_next_available_cookie_id(next_free_id);
		output::log(output::loglevel::INFO, "flow_service::flow_update_handler() initial cookie ID is %" PRIu64 ".\n", next_free_id);
	}

	// update all entries atomically
	uint32_t total_flows_installed = 0;
	uint32_t flows_installed = 0;
	uint32_t flows_received = pending_flows.size();
	for (auto& flow_table : flow_tables) {
		flows_installed = flow_table->update_entries(pending_flows);
		output::log(output::loglevel::INFO, "flow service: %u flow(s) updated in table %s.\n", flows_installed, flow_table->get_table_name().c_str());
		total_flows_installed += flows_installed;
	}

	if (total_flows_installed != flows_received) {
		output::log(output::loglevel::BUG, "flow service error: table install disparity -- %u received, %u installed.\n", flows_received, flows_installed);
		abort();
	} else {
		output::log(output::loglevel::INFO, "flow service: %u flows refreshed.\n", total_flows_installed);
	}
	pending_flows.clear();
}

// removes a flow from the flow table
void flow_service::flow_update_handler(const of_message_flow_removed& msg) {

	bool found = false;
	for (auto& flow_table : flow_tables) {
		if (flow_table->is_cookie_used(msg.cookie)) {
			found = true;
			flow_table->mark_entry_as_deleted(msg.cookie);
		}
	}

	if (initialized && !found) {
		output::log(output::loglevel::ERROR, "flow_service::flow_update_handler() switch reported a flow being deleted but the flow does not exist in the table.\n");
		output::log(output::loglevel::ERROR, "the flow that caused this error is:\n%s\n", msg.to_string().c_str());
	}
}

// returns service information
string flow_service::get_service_info() const {
	char buf[128];
	sprintf(buf,"generic ironstack flow table management service version %d.%d.", service_major_version, service_minor_version);
	return string(buf);
}

// returns running information about the service
string flow_service::get_running_info() const {
	return string();
}

// setup a pointer to the controller
void flow_service_port_mod_callback::init(hal* controller_) {
	controller = controller_;
}

// callback object for when the switch state detects a change in port status
void flow_service_port_mod_callback::switch_port_modification_callback(const openflow_vlan_port& port_) {

	// check port status. don't do anything if the port came online
	auto port = port_;
	bool port_online = port.get_openflow_port().is_up();
	if (port_online) {
		return;

	// handle only the case when the port goes offline
	} else if (!port.is_valid() || !port_online) {

		if (controller != nullptr) {
			shared_ptr<cam> cam_svc = static_pointer_cast<cam>(controller->get_service(service_catalog::service_type::CAM));
			shared_ptr<arp> arp_svc = static_pointer_cast<arp>(controller->get_service(service_catalog::service_type::ARP));
			shared_ptr<flow_service> flow_svc = static_pointer_cast<flow_service>(controller->get_service(service_catalog::service_type::FLOWS));
			shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
			shared_ptr<inter_ironstack_service> iis = static_pointer_cast<inter_ironstack_service>(controller->get_service(service_catalog::service_type::INTER_IRONSTACK));
			list<mac_address> l2_addresses_to_purge;

			// flush all CAM entries on this port
			if (cam_svc != nullptr) {
				cam_svc->remove(port.get_openflow_port().port_number);
			}

			// get a list of all flows
			auto all_flows = flow_svc->get_flows();

			// every flow that contains this output port should be removed
			for (const auto& flow : all_flows) {

				// ignore flows that are already in the process of being deleted
				if (flow.state != openflow_flow_entry::flow_state::PENDING_INSTALLATION
					&& flow.state != openflow_flow_entry::flow_state::ACTIVE) {
					continue;
				}

				auto action_list = flow.description.action_list.get_actions();
				bool to_delete = false;
				bool is_l2_flow = false;
				uint16_t vlan_id=0;
				mac_address dl_address;

				// check if this flow should be deleted on the basis of its port getting invalidated
				// TODO -- to be more generic, should analyze each action set and remove only the action that outputs to this port
				if (action_list.size() == 1) {
					if (action_list.front()->get_action_type() == of_action::action_type::OUTPUT_TO_PORT) {
						of_action_output_to_port* ptr = (of_action_output_to_port*) action_list.front().get();
						if (ptr->port == port.get_openflow_port().port_number) {
							// should delete this flow
							to_delete = true;

							// is this an L2 flow?
							const of_match& criteria = flow.description.criteria;
							if (!criteria.wildcard_ethernet_dest &&
								!criteria.wildcard_vlan_id &&
								criteria.wildcard_in_port &&
								criteria.wildcard_ethernet_src &&
								criteria.wildcard_vlan_pcp &&
								criteria.wildcard_ethernet_frame_type &&
								criteria.wildcard_ip_type_of_service &&
								criteria.wildcard_ip_protocol &&
								criteria.wildcard_ip_src_lsb_count == 32 &&
								criteria.wildcard_ip_dest_lsb_count == 32 &&
								criteria.wildcard_tcpudp_src_port &&
								criteria.wildcard_tcpudp_dest_port) {

								is_l2_flow = true;
								vlan_id = criteria.vlan_id;
								dl_address = criteria.ethernet_dest;
							}
						}

						// remove affected flow since output port is the port that has been taken down
						if (to_delete) {
							output::log(output::loglevel::INFO, "removing [%s] because port %hu is down.\n", flow.to_string().c_str(), port.get_openflow_port().port_number);
							flow_svc->remove_flow(flow.description.cookie);
						}

						// for L2 flows, batch the addresses so we can later inform other controllers to invalidate the entries
						// also remove the entry from the ARP table
						if (is_l2_flow) {
							l2_addresses_to_purge.push_back(dl_address);

							if (arp_svc != nullptr) {
								arp_svc->remove(dl_address, vlan_id);
							}
						}
					}
				}
			}

			// submit the batch of addresses to the inter-ironstack service for broadcast to all controllers
			if (!l2_addresses_to_purge.empty() && iis != nullptr) {
				iis->announce_invalidate_mac(l2_addresses_to_purge);
			}

		}
	}
}

