#pragma once

#include "../hal/hal_transaction.h"

#include <map>
#include <stdint.h>
#include "../ironstack_types/openflow_flow_entry.h"
#include "../openflow_messages/of_message_error.h"
#include "../openflow_messages/of_message_flow_removed.h"
#include "../openflow_messages/of_message_modify_flow.h"
using namespace std;

// generic flow table superclass
// note: these tables are data storage only, they do not talk to the switch
// synchronization is provided within each table
class flow_table : public hal_callbacks {
public:

	flow_table():max_capacity(0) {}

	void         set_max_capacity(uint32_t max);
	uint32_t     get_max_capacity() const;
	uint32_t     get_used_capacity();
	uint32_t     get_available_capacity();

	// clears the table but does not affect flow entries on the switch
	void         clear_table(bool include_static=false);
	
	// check if a flow will map to this table
	bool         check_table_fit(const openflow_flow_description& flow) const;
	virtual bool check_table_fit(const of_match& criteria,
                 const openflow_action_list& action_list) const=0;

	// check if a cookie ID is already used
	// this is purely to check if a cookie ID is used in the table. this function
	// should not be used to check if a flow is installed.
	bool         is_cookie_used(uint64_t cookie) const;

	// gets the complete flow status on a flow by cookie ID
	bool         get_flow_entry_by_cookie(uint64_t cookie, openflow_flow_entry& result) const;

	// checks if a flow is currently installed with the flow descriptions
	// flows that are pending delete or pending install will still be considered
	// installed. cookie IDs are 0xffffffffffffffff (ie -1) if not installed.
	uint64_t     get_cookie_for_flow(const openflow_flow_description& flow) const;
	uint64_t     get_cookie_for_flow(const of_match& criteria,
                 const openflow_action_list& action_list) const;

	// adds an entry to the flow table
	// returns false if flow capacity is reached.
	bool         add_entry(const openflow_flow_entry& entry);

	// updates all entries in the table, returns number of flows installed
	// removes entries that were absorbed by the table (so the next table won't see them)
	uint32_t     update_entries(vector<openflow_flow_description_and_stats>& flows);

	// marks a flow as 'installed' only if it was previously pending install
	// a flow in any other state (active, pending delete, deleted or unknown) stays unchanged.
	bool         mark_entry_as_installed(uint64_t cookie_id);

	// marks an entry as 'pending deletion' if it was previously in any state other than
	// deleted.
	// a flow in the deleted state stays unchanged. this is noted as a bug though.
	bool         mark_entry_as_pending_deletion(uint64_t cookie_id);

	// marks an entry as deleted, regardless of its previous state. this is usually called
	// in response to a flow removed message received from the switch.
	bool         mark_entry_as_deleted(uint64_t cookie_id);

	// marks all flows as pending delete (include_static flag controls if all
	// flows are set or if only non-static flows are marked)
	// returns the flows that are slated for deletion (ie, flows in the pending installation or
	// active state).
	vector<openflow_flow_entry> mark_all_flows_as_pending_delete(bool include_static);

	// gets all the flows in this table
	vector<openflow_flow_entry> get_all_flows(bool include_static=true) const;
	map<uint64_t, openflow_flow_entry> get_all_flows_map() const;
	
	// gets the name of the table
	virtual string get_table_name() const=0;

	// used by the flow service to atomically update 

	// used to handle callbacks from HAL to update transactions that have
	// completed
	virtual void hal_callback(const shared_ptr<hal_transaction>& transaction,
		const shared_ptr<of_message>& reply, bool status);

	// used by the flow service to set the next available cookie id
	static void     set_next_available_cookie_id(uint64_t next_cookie);

	// gets the next available cookie
	static uint64_t get_next_available_cookie_id();

protected:

	// per table information
	mutable mutex                      table_lock;
	uint32_t                           max_capacity;
	map<uint64_t, openflow_flow_entry> flows;

	// static cookie counters for all flow tables
	static mutex                       cookie_lock;
	static uint64_t                    next_available_cookie_id;

	// used to decide how long deleted entries will persist in the listing
	static const int                   DELETED_ENTRY_TIMEOUT = 5000;
};
