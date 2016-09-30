#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include "cam.h"
#include "flow_table.h"
#include "switch_state.h"
#include "../ironstack_types/openflow_flow_entry.h"
#include "../openflow_messages/of_message_flow_removed.h"
#include "../openflow_messages/of_message_stats_reply.h"
#include "../hal/hal.h"
#include "../hal/packet_in_processor.h"
#include "../hal/service_catalog.h"
#include "../hal/hal_transaction.h"
class cam;
class of_message_flow_removed;
class switch_state;
class flow_service_port_mod_callback;
using namespace std;


// the flow service class manages the addition, modification, and deletion of flows.
// it does not track statistics.
class flow_service : public service {
public:

	flow_service(service_catalog* ptr):
		service(ptr, service_catalog::service_type::FLOWS, 2, 0),
		initialized(false),
		initializing(false),
		last_stats_update_xid(0) { 

		dependencies = { service_catalog::service_type::CAM,
			service_catalog::service_type::ARP,
			service_catalog::service_type::SWITCH_STATE };

		controller = ptr->get_controller();
	}

	virtual ~flow_service() {};

	// startup and shutdown functions
	virtual bool init();
	virtual bool init2();
	virtual void shutdown();

	// functions to attach tables
	// note: attachment order determines the order in which tables are consulted first
	//       so make sure ACL tables are attached last
	void attach_flow_table(const shared_ptr<flow_table>& table);
	void detach_flow_table(const shared_ptr<flow_table>& table);

	// syncs flows with switch
	// timeout is in milliseconds. possible values are:
	// 0 : nonblocking function call (always returns immediately; returns true if
	//     request was issued to controller and false if not initialized).
	// -1: request will block indefinitely (not recommended).
  // +t: will issue the request and wait for up to t milliseconds to complete.
	bool refresh_flows(int timeout_ms=0);
	
	// clears all flows on the switch and internal data structures.
	// timeout is in milliseconds. possible values are:
	// 0 : nonblocking.
	// -1: request will block indefinitely (not recommended).
	// +t: will issue the request and wait up to t milliseconds to complete.
	bool clear_all_flows(int timeout_ms=0, bool keep_static_flows=true);

	// adds a flow, automatically setting up priority and cookies. returns cookie ID
	// setting a positive timeout will cause the provisional status to be known after the timeout
	// but a failure does not mean that the action did not complete successfully.
	//
	// specifying an idle or hard timeout of 0 will make the flow permanent (not
	// subject to ageing out on the switch).
	//
	// if a flow already exists, installing it is a no-op. the timers are not
	// refreshed by doing the install.
	//
	// timeout is in milliseconds. possible values are:
	// 0 : nonblocking.
	// -1: request will block indefinitely (not recommended).
	// +t: will issue the request and wait for up to t milliseconds to complete.
	uint64_t add_flow_auto(const of_match& criteria, const openflow_action_list& action_list, const string& reason, bool is_static=false, int install_timeout_ms=-1);
	uint64_t add_flow_auto(const of_match& criteria, const openflow_action_list& action_list, const string& reason, uint16_t priority, bool is_static=false, int install_timeout_ms=-1);

	// adds a flow (completely specified). returns cookie ID
	uint64_t add_flow(const openflow_flow_description& flow, const string& reason, uint16_t idle_timeout, uint16_t hard_timeout, bool is_static=false, int install_timeout_ms=-1);

	// removes a specific flow. this is the strict matching criteria
	uint64_t remove_flow_strict(const openflow_flow_description& flow);

	// removes a flow by cookie ID
	uint64_t remove_flow(uint64_t cookie_id);

	// checks if a flow exists by ID
	uint64_t does_flow_exist(uint64_t cookie_id);

	// checks if a flow exists. strict matching criteria
	uint64_t does_flow_exist_strict(const openflow_flow_description& flow) const;

	// gets the vector/map of all flows (potentially expensive copy opereation)
	vector<openflow_flow_entry> get_flows() const;
	map<uint64_t, openflow_flow_entry> get_flows_map() const;

	// packet handling functions
	void flow_update_handler(const of_message_stats_reply_flow_stats& stats);
	void flow_update_handler(const of_message_flow_removed& msg);

	// required from service
	virtual string get_service_info() const;
	virtual string get_running_info() const;

private:

	mutable mutex                   lock;
	bool                            initialized;
	bool                            initializing;

	uint32_t                        last_stats_update_xid;
	vector<shared_ptr<flow_table>>  flow_tables;	// flow tables can only be added/removed when
	                                              // the system is not initialized. after initialization
                                                // the vector is read-only and not protected by a mutex
                                                // (but the flow tables within are thread safe)

	vector<openflow_flow_description_and_stats> pending_flows;  // used to hold flow tables that are
	                                                            // in the process of being downloaded.
																						 	                // flow_update handler will atomically update
																					          					// all flow tables when the no_more_next flag is set
																							           		  // on the flow reply message

	shared_ptr<flow_service_port_mod_callback>  port_cob;       // callback object for port changes

	const uint16_t default_priority       = 100;
	const uint16_t default_idle_timeout   = 0; // TODO: made permanent for now  // 10 minutes
	const uint16_t default_hard_timeout   = 0; // TODO: made permanent for now // half an hour

};

// a helper class for handling callbacks
class flow_service_port_mod_callback : public switch_port_modification_callbacks {
public:

	// setup ptr to controller
	void init(hal* controller);

	// callback for handling flows that need to be removed on port disconnection
	virtual void switch_port_modification_callback(const openflow_vlan_port& port);

private:

	hal* controller;
};

