#pragma once
#include "../../common/timer.h"
#include "openflow_flow_description.h"
class flow_table;
using namespace std;

class openflow_flow_entry {
public:

	enum class flow_state { UNKNOWN,
													PENDING_INSTALLATION,
													ACTIVE,
													PENDING_DELETION,
													DELETED
												};

	openflow_flow_entry();

	// resets the data on this flow entry
	void   clear();

	// sets the state and updates the timer
	void   set_state(const flow_state& new_state);

	// getter functions
	bool   is_live() const;   // live means not currently undergoing install or
                            // removal
	bool   is_static() const; // static means the flow will not be aged out 

	// returns the time since the status was updated
	int    get_time_since_update_ms() const;

	// generates a readable version of this flow
	string to_string() const;

	// generates a string version of the state
	string get_state_string() const;

	// user-accessible fields
	flow_state                state;    
	uint32_t                  idle_timeout;
	uint32_t                  hard_timeout;
	timer                     last_updated;
	string                    install_reason;
  openflow_flow_description description;

private:

	friend class flow_table;

	// reserved for flow table to use during an update operation
	// used to flag if a flow has been updated into the table during a flow refresh
	bool                      is_updated;

};
