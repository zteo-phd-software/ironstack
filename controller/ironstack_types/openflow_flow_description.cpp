#include "openflow_flow_description.h"

// constructor
openflow_flow_description::openflow_flow_description() {
	cookie = 0;
	priority = 0;
}

// resets the object
void openflow_flow_description::clear() {
	criteria.clear();
	action_list.clear();
	cookie = 0;
	priority = 0;
}

// checks for the equality of both objects compared
// flows are the same if their match/action properties are the same.
// however note that cookie IDs have to be unique! the equality function does not check
// for that. priorities are also not checked.
bool openflow_flow_description::operator==(const openflow_flow_description& other) const {
	if (criteria == other.criteria &&
		action_list == other.action_list) {
		return true;
	}

	return false;
}

bool openflow_flow_description::operator!=(const openflow_flow_description& other) const {
	return !(*this == other);
}

std::string openflow_flow_description::to_string() const {
	std::string result;
	char buf[64];

	result += criteria.to_string();

	result += "\ncookie id: ";
	snprintf(buf, sizeof(buf), "%" PRIu64, cookie);
	result += buf;

	result += "\npriority : ";
	snprintf(buf, sizeof(buf), "%u", priority);
	result += buf;

	result += "\nactions  : ";
	result += action_list.to_string();
	
	return result;
}

uint32_t openflow_flow_description::serialize(autobuf& dest) const {

	// TODO -- incomplete
	// need to implement this function for remote rule installation
	return 0;
}

bool openflow_flow_description::deserialize(const autobuf& content) {
	// TODO -- incomplete
	// need to implement this function for remote rule installation
	return false;
}
