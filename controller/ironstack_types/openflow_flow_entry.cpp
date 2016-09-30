#include "openflow_flow_entry.h"

// constructor
openflow_flow_entry::openflow_flow_entry() {
	state = flow_state::UNKNOWN;
	idle_timeout = 0;
	hard_timeout = 0;
	install_reason = "unknown";
	is_updated = false;
}

// clears the object
void openflow_flow_entry::clear() {
	state = flow_state::UNKNOWN;
	idle_timeout = 0;
	hard_timeout = 0;
	last_updated.clear();
	install_reason = "unknown";
	description.clear();
	is_updated = false;
}

// sets a new state and resets the update timer
void openflow_flow_entry::set_state(const flow_state& new_state) {
	state = new_state;
	last_updated.reset();
}

// check if the flow is actually alive and stable (not being installed/removed)
bool openflow_flow_entry::is_live() const {
	return state == flow_state::ACTIVE;
}

// checks if the flow is static (can't be aged out)
bool openflow_flow_entry::is_static() const {
	return idle_timeout == 0 && hard_timeout == 0;
}

// gets the time in ms since the flow table received an update on the status of this flow
int openflow_flow_entry::get_time_since_update_ms() const {
	return last_updated.get_time_elapsed_ms();
}

// generates a readable version of the flow entry
string openflow_flow_entry::to_string() const {
	
	string result;
	char buf[128];
	sprintf(buf, "static flow   : %s\n", is_static() ? "yes" : "no");
	result += buf;
	result += "flow status   : ";
	result += get_state_string();
	sprintf(buf, "\nidle timeout  : %us\n", idle_timeout);
	result += buf;
	sprintf(buf, "hard timeout  : %us\n", hard_timeout);
	result += buf;
	sprintf(buf, "last updated  : %us\n", last_updated.get_time_elapsed_ms()/1000);
	result += buf;
	result +=    "install reason: ";
	result += install_reason;
	result += "\n";
	result += description.to_string();

	return result;
}

// returns a string corresponding to the state of the flow entry
string openflow_flow_entry::get_state_string() const {
	switch (state) {
		case flow_state::UNKNOWN:
			return string("unknown");
		case flow_state::PENDING_INSTALLATION:
			return string("pending installation");
		case flow_state::ACTIVE:
			return string("active");
		case flow_state::PENDING_DELETION:
			return string("pending deletion");
		case flow_state::DELETED:
			return string("deleted");
		default:
			return string("invalid??");
	}
}
