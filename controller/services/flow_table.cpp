#include "flow_table.h"
#include "../gui/output.h"
using namespace std;

extern string switch_name;

// static variables here
mutex flow_table::cookie_lock;
uint64_t flow_table::next_available_cookie_id = 1; //((uint64_t) -1);

// set up maximum table capacity
void flow_table::set_max_capacity(uint32_t max) {
	lock_guard<mutex> g(table_lock);
	max_capacity = max;
}

// returns maximum table capacity
uint32_t flow_table::get_max_capacity() const {
	lock_guard<mutex> g(table_lock);
	return max_capacity;
}

// returns number of entries used
uint32_t flow_table::get_used_capacity() {
	lock_guard<mutex> g(table_lock);

	uint32_t result = 0;
	auto iterator = flows.begin();
	while (iterator != flows.end()) {
		if (iterator->second.state != openflow_flow_entry::flow_state::DELETED) {
			++iterator;
			++result;
		} else if (iterator->second.get_time_since_update_ms() >= DELETED_ENTRY_TIMEOUT) {
			iterator = flows.erase(iterator);
		}
	}

	return result;
}

// returns number of flow entries available for this table
uint32_t flow_table::get_available_capacity() {
	lock_guard<mutex> g(table_lock);
	
	uint32_t used = 0;
	auto iterator = flows.begin();
	while (iterator != flows.end()) {
		if (iterator->second.state != openflow_flow_entry::flow_state::DELETED) {
			++iterator;
			++used;
		} else if (iterator->second.get_time_since_update_ms() >= DELETED_ENTRY_TIMEOUT) {
			iterator = flows.erase(iterator);
		}
	}

	if (max_capacity > used) {
		return max_capacity - used;
	} else {
		return 0;
	}
}

// clears all entries in the table
void flow_table::clear_table(bool include_static) {
	lock_guard<mutex> g(table_lock);
	if (!include_static) {
		flows.clear();
	} else {
		auto iterator = flows.begin();
		while (iterator != flows.end()) {
			if (!iterator->second.is_static()) {
				iterator = flows.erase(iterator);
			} else {
				++iterator;
			}
		}
	}
}

// checks if a flow is eligible for installation in this table
// checks only criteria and actions (flow overlaps and cookies are not checked here)
bool flow_table::check_table_fit(const openflow_flow_description& flow) const {
	return check_table_fit(flow.criteria, flow.action_list);
}

// checks if a cookie ID is already in use in this table
bool flow_table::is_cookie_used(uint64_t cookie) const {
	lock_guard<mutex> g(table_lock);
	return flows.count(cookie) > 0;
}

// looks up an openflow entry by cookie id
bool flow_table::get_flow_entry_by_cookie(uint64_t cookie, openflow_flow_entry& result) const {
	lock_guard<mutex> g(table_lock);
	auto iterator = flows.find(cookie);
	if (iterator != flows.end()) {
		result = iterator->second;
		return true;
	} else {
		result.clear();
		return false;
	}
}

/*
// returns the openflow entry (with install/delete status)
openflow_flow_entry flow_table::get_flow_entry_status(uint64_t cookie) const {
	openflow_flow_entry result;
	lock_guard<mutex> g(table_lock);
	auto iterator = flows.find(cookie);
	if (iterator != flows.end()) {
		result = iterator->second;
	}
	return result;
}
*/

// returns the cookie id for a given flow description. if the cookie is set,
// the search will check based on the cookie ID. otherwise this is a linear search
// and is potentially very slow!
uint64_t flow_table::get_cookie_for_flow(const openflow_flow_description& flow) const {

	lock_guard<mutex> g(table_lock);

	// look for the most recent flow matching the description
	uint64_t result = ((uint64_t) -1);
	bool found = false;
	for (const auto& one_flow : flows) {
		if (one_flow.second.description == flow) {
			if (!found) {
				found = true;
				result = one_flow.second.description.cookie;
//				output::log(output::loglevel::INFO, "flow_table::get_cookie_for_flow() -- found cookie %" PRIu64 " state: %s\n", result, one_flow.second.get_state_string().c_str());

			// take the latest cookie ID, since that is the most recent flow entry
			} else if (one_flow.second.description.cookie > result) {
				result = one_flow.second.description.cookie;
//				output::log(output::loglevel::INFO, "flow_table::get_cookie_for_flow() -- found cookie %" PRIu64 " state: %s\n", result, one_flow.second.get_state_string().c_str());
			}
		}
	}

	return result;
}

// returns the cookie id for a given flow match criteria and action list
uint64_t flow_table::get_cookie_for_flow(const of_match& criteria, const openflow_action_list& action_list) const {
	openflow_flow_description description;
	description.criteria = criteria;
	description.action_list = action_list;
	description.cookie = 0;
	description.priority = 0;
	return get_cookie_for_flow(description);
}

// adds an entry into the flow table if it doesn't exist.
// automatically assigns a cookie ID if one was not supplied.
// table fitting checks are not done here. if a failure occurs, this is due to a capacity problem.
bool flow_table::add_entry(const openflow_flow_entry& entry) {

	// if no cookie was given, this is a bug!
	if (entry.description.cookie == 0 || entry.description.cookie == ((uint64_t)-1)) {
		output::log(output::loglevel::BUG, "flow_table::add_entry() -- cookie was not assigned!\n");
		abort();
	}

	lock_guard<mutex> g(table_lock);
	if (flows.size() >= max_capacity) {
		return false;
	} else {
		flows[entry.description.cookie] = entry;
		return true;
	}
}

// updates multiple entries in the table. returns number of flows installed
uint32_t flow_table::update_entries(vector<openflow_flow_description_and_stats>& updated_flows) {
	uint32_t result = 0;
	uint64_t cookie;

	lock_guard<mutex> g(table_lock);
	
	// iterate over each flow, make sure it fits within table
	auto updated_flows_iterator = updated_flows.begin();
	while (updated_flows_iterator != updated_flows.end() && check_table_fit(updated_flows_iterator->flow_description)) {

		// check if this flow is in the existing table
		const auto& updated_flow_description = updated_flows_iterator->flow_description;
		cookie = updated_flow_description.cookie;
		auto table_iterator = flows.find(cookie);
		if (table_iterator != flows.end()) {

			// update flow since it was already in the table
			auto& table_flow = table_iterator->second;
			table_flow.is_updated = true;
			switch (table_flow.state) {
				case openflow_flow_entry::flow_state::UNKNOWN:
				case openflow_flow_entry::flow_state::PENDING_INSTALLATION:
					table_flow.set_state(openflow_flow_entry::flow_state::ACTIVE);
					break;
				case openflow_flow_entry::flow_state::ACTIVE:							// flows in active or pending deletion state don't change state
				case openflow_flow_entry::flow_state::PENDING_DELETION:
					break;
				case openflow_flow_entry::flow_state::DELETED:
					output::log(output::loglevel::BUG, "flow_table::update_entries() -- flow marked as deleted but it is seen again on refresh!\n");
					break;
				default:
					output::log(output::loglevel::BUG, "flow_table::update_entries() -- flow state check: unknown state encountered.\n");
					break;
			}

		// flow was never in the table but the switch reported it. must have been inherited from a previous controller
		// instance
		} else {

			// create new flow entry
			openflow_flow_entry entry;
			entry.state = openflow_flow_entry::flow_state::ACTIVE;
			entry.idle_timeout = updated_flows_iterator->duration_to_idle_timeout;
			entry.hard_timeout = updated_flows_iterator->duration_to_hard_timeout;
			entry.description = updated_flows_iterator->flow_description;
			entry.install_reason = "inherited";
			entry.is_updated = true;
			
			flows[cookie] = move(entry);
		}

		// the flow has been processed for this table and should be removed so the next table won't see this flow
		updated_flows_iterator = updated_flows.erase(updated_flows_iterator);
		++result;
	}

	// finished processing all entries for this table. mark entries that are no longer seen as deleted
	auto flow_iterator = flows.begin();
	while (flow_iterator != flows.end()) {
		auto& current_flow = flow_iterator->second;

		// flows that were not flagged as updated during this cycle are deemed to have been deleted
		if (!current_flow.is_updated) {
		
			if (current_flow.state == openflow_flow_entry::flow_state::PENDING_DELETION) {
				current_flow.set_state(openflow_flow_entry::flow_state::DELETED);
				++flow_iterator;

			// let deleted flows linger around for a while until we purge them (after 5 seconds)
			} else if (current_flow.state == openflow_flow_entry::flow_state::DELETED && current_flow.last_updated.get_time_elapsed_ms() > DELETED_ENTRY_TIMEOUT) {
				flow_iterator = flows.erase(flow_iterator);

			// flow wasn't marked as deleted, but it disappeared!
			} else {
				output::log(output::loglevel::BUG, "flow_table::update_entries() -- flow was not designated for removal but does not appear in update list! the flow was [%s].\n",
					current_flow.to_string().c_str());
				flow_iterator = flows.erase(flow_iterator);
			}

		// reset the flag for the next refresh cycle
		} else {
			current_flow.is_updated = false;
			++flow_iterator;
		}

	}

	// return number of flows eaten by this table
	return result;
}

// flags a pending flow entry as installed
bool flow_table::mark_entry_as_installed(uint64_t cookie_id) {
	lock_guard<mutex> g(table_lock);
	auto iterator = flows.find(cookie_id);
	if (iterator == flows.end()) {
		return false;
	}
	
	// sanity check
	switch (iterator->second.state) {

		// if pending, mark as active and fall through
		case openflow_flow_entry::flow_state::PENDING_INSTALLATION:
			iterator->second.set_state(openflow_flow_entry::flow_state::ACTIVE);

		// fall through. in the active or pending deletion states, marking an entry as installed is a no-op.
		case openflow_flow_entry::flow_state::ACTIVE:
		case openflow_flow_entry::flow_state::PENDING_DELETION:
			return true;
	
		// marking an entry as installed when the flow state is unknown/deleted is a bug.
		default:
			output::log(output::loglevel::BUG, "flow_table::mark_entry_as_installed() -- flow state inconsistent [unknown or deleted] when it should be active.\n");
			break;
	}
	return false;
}

// flags a flow entry for removal from the table
// the callback from HAL will confirm the removal and it will be marked as deleted at that time.
bool flow_table::mark_entry_as_pending_deletion(uint64_t cookie_id) {

	lock_guard<mutex> g(table_lock);
	auto iterator = flows.find(cookie_id);
	if (iterator == flows.end()) {
		return false;
	} else if (iterator->second.state == openflow_flow_entry::flow_state::DELETED) {
		output::log(output::loglevel::BUG, "flow_table::mark_entry_for_removal() -- flow already deleted when it was reflagged for removal. flow details: [%s]\n",
			iterator->second.description.to_string().c_str());
		return false;
	}

	iterator->second.set_state(openflow_flow_entry::flow_state::PENDING_DELETION);
	return true;
}

// marks an entry as 'deleted', regardless of previous state
// the flow update handler will clear out the marked entry eventually (after 5 seconds)
bool flow_table::mark_entry_as_deleted(uint64_t cookie_id) {
	lock_guard<mutex> g(table_lock);
	auto iterator = flows.find(cookie_id);
	if (iterator == flows.end()) return false;

	iterator->second.set_state(openflow_flow_entry::flow_state::DELETED);
	return true;
}

// mark all entries as pending delete (if include_static is set to false,
// only non-static flows are marked
// returns all flows slated for deletion (flows that were in the pending install/installed state only)
vector<openflow_flow_entry> flow_table::mark_all_flows_as_pending_delete(bool include_static) {
	vector<openflow_flow_entry> result;
	lock_guard<mutex> g(table_lock);
	result.reserve(flows.size());

	for (auto& flow : flows) {
		if (!include_static && flow.second.is_static()) continue;

		if (flow.second.state == openflow_flow_entry::flow_state::PENDING_INSTALLATION
			|| flow.second.state == openflow_flow_entry::flow_state::ACTIVE) {
			flow.second.set_state(openflow_flow_entry::flow_state::PENDING_DELETION);
			result.push_back(flow.second);
		}
	}

	return result;
}

// returns a vector of all the flows in this table
vector<openflow_flow_entry> flow_table::get_all_flows(bool include_static) const {
	lock_guard<mutex> g(table_lock);
	vector<openflow_flow_entry> result;
	result.reserve(flows.size());
	for (const auto& flow : flows) {
		if (include_static) {
			result.push_back(flow.second);
		} else if (!flow.second.is_static()) {
			result.push_back(flow.second);
		}
	}
	return result;
}

// returns the map of all flows in this table
map<uint64_t, openflow_flow_entry> flow_table::get_all_flows_map() const {
	lock_guard<mutex> g(table_lock);
	return flows;
}

// handles callbacks from HAL to update transactions that have completed
void flow_table::hal_callback(const shared_ptr<hal_transaction>& transaction,
	const shared_ptr<of_message>& reply, bool status) {

	if (transaction->get_request()->msg_type == OFPT_FLOW_MOD) {

		shared_ptr<of_message_modify_flow> msg = static_pointer_cast<of_message_modify_flow>(transaction->get_request());
		if (msg->command == OFPFC_ADD || msg->command == OFPFC_MODIFY || msg->command == OFPFC_MODIFY_STRICT) {

			// this is an add command. make sure there was no error
			if (reply == nullptr) {

				// add or modify rule was successful. set the openflow entry flag
				if (mark_entry_as_installed(msg->flow_description.cookie)) {
					output::log(output::loglevel::INFO, "flow_table::hal_callback() -- flow added successfully:\n%s.\n",
						msg->flow_description.to_string().c_str());
				} else {
					output::log(output::loglevel::BUG, "flow_table::hal_callback() add flow successful, but could not mark table entry as active:\n%s.\n",
						msg->flow_description.to_string().c_str());
				}

			// add or modify rule was unsuccessful. remove from the flow table
			} else if (reply->msg_type == OFPT_ERROR) {
				
				output::log(output::loglevel::ERROR, "flow_table::hal_callback() -- flow could not be added or modified. the faulting flow is:\n%s\n",
					msg->flow_description.to_string().c_str());
				mark_entry_as_deleted(msg->flow_description.cookie);	// TODO -- should technically create an 'error' state
			} else {
				output::log(output::loglevel::BUG, "flow_table::hal_callback() -- unexpected reply type? the flow and reply were:\n%s\nflow reply:\n%s\n", 
					msg->flow_description.to_string().c_str(),
					reply->to_string().c_str());
			}

		} else {

			// this is a delete command
			if (reply == nullptr || reply->msg_type == OFPT_FLOW_REMOVED) {

				// remove was successful. mark the flow as deleted in the table
				output::log(output::loglevel::INFO, "flow_table::hal_callback() -- flow removed:\n%s\n", msg->flow_description.to_string().c_str());
				mark_entry_as_deleted(msg->flow_description.cookie);

			} else if (reply->msg_type == OFPT_ERROR) {
				output::log(output::loglevel::BUG, "flow_table::hal_callback() -- flow could not be removed. the faulting flow is:\n[%s]\n",
					msg->flow_description.to_string().c_str());
			} else {
				output::log(output::loglevel::BUG, "flow_table::hal_callback() -- unexpected reply type? the reply was:\n%s\n", reply->to_string().c_str());
			}
		}

	} else {
		output::log(output::loglevel::BUG, "flow_table::hal_callback() -- unexpected reply. the transaction received was:\n%s\n", transaction->get_request()->to_string().c_str());
		assert(0);
		abort();
	}
}

// set the next available cookie ID (static variable) atomically
void flow_table::set_next_available_cookie_id(uint64_t next_cookie) {
	lock_guard<mutex> g(cookie_lock);
	next_available_cookie_id = next_cookie;
}

// gets the next available cookie
uint64_t flow_table::get_next_available_cookie_id() {
	lock_guard<mutex> g(cookie_lock);
	if (next_available_cookie_id == ((uint64_t) -1)) {
		output::log(output::loglevel::BUG, "[%s] flow_table::get_next_available_cookie_id() -- out of cookie IDs or failed to initialize cookie IDs.\n", switch_name.c_str());
		output::log(output::loglevel::BUG, "this bug can be remedied. please report to the IronStack dev team.\n");

		FILE* fp = fopen("/dev/random", "r");
		if (fp != nullptr) {
			uint8_t rand_buf[8];
			for (uint32_t counter = 0; counter < 8; ++counter) {
				int v = fgetc(fp);
				rand_buf[counter] = (uint8_t) v;
			}
			next_available_cookie_id = *((uint64_t*) rand_buf);

		} else {
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			srand(tv.tv_sec + tv.tv_usec);
			next_available_cookie_id = (((uint64_t)rand()) << 32) + (uint64_t) rand();
		}
		return next_available_cookie_id;
	}
	return next_available_cookie_id++;
}
