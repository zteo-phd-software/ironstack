#include "openflow_action_list.h"
#include "../gui/output.h"

// constructor
openflow_action_list::openflow_action_list() {
}

// destructor
openflow_action_list::~openflow_action_list() {
}

// copy constructor 
openflow_action_list::openflow_action_list(const openflow_action_list& original) {
	for (const auto& action : original.actions) {
		actions.push_back(std::move(std::unique_ptr<of_action>(of_action::clone(action.get()))));
	}
}

// assignment operator
openflow_action_list& openflow_action_list::operator=(const openflow_action_list& original) {
	clear();
	for (const auto& action : original.actions) {
		actions.push_back(std::move(std::unique_ptr<of_action>(of_action::clone(action.get()))));
	}

	return *this;
}

// checks if action lists are identical
bool openflow_action_list::operator==(const openflow_action_list& other) const {
	if (actions.size() != other.actions.size()) {
		return false;
	}

	bool found;
	for (const auto& action : actions) {

		found = false;
		for (const auto& other_action : other.actions) {
			if (*action == *other_action) {
				found = true;
				break;
			}
		}

		if (!found) {
			return false;
		}
	}

	return true;
}

// checks if action lists are different
bool openflow_action_list::operator!=(const openflow_action_list& other) const {
	return !(*this == other);
}

// clears the action list
void openflow_action_list::clear() {
	actions.clear();
}

// returns a readable form of the action list
std::string openflow_action_list::to_string() const {
	std::string result;
	if (actions.size() == 0) {
		result += "no actions.";
	}

	for (const auto& item : actions) {
		result += item->to_string() + string(" ");
	}

	return result;
}

// generates the action list based on a string input
bool openflow_action_list::from_string(const std::string& input) {

	clear();
	std::vector<std::pair<std::string, std::string>> kv = string_utils::generate_key_value_pair(input);
	bool result = true;
	std::unique_ptr<of_action> action_to_add;
	uint32_t max = kv.size();

	for (uint32_t index = 0; index < max; ++index) {
		if (kv[index].first == "out_port") {
			uint16_t port;
			if (kv[index].second == "controller") {
				port = (uint16_t) OFPP_CONTROLLER;
			} else if (!string_utils::string_to_uint16(kv[index].second, &port)) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse out_port [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_output_to_port* action = new of_action_output_to_port();
			action->send_to_controller = (port == (uint16_t) OFPP_CONTROLLER);
			action->port = port;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "enqueue_port") {
			uint16_t port;
			uint32_t queue_id;
			if (!string_utils::string_to_uint16(kv[index].second, &port)) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse enqueue_port [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			} else {
				if (++index >= max || kv[index].first != "queue_id") {
					output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse enqueue_port; no queue_id.\n");
					result = false;
					continue;
				} else if (!string_utils::string_to_uint32(kv[index].second, &queue_id)) {
					output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse enqueue_port action queue_id [%s]; ignoring.\n", kv[index].second.c_str());
					result = false;
					continue;
				}
			
				// synthesize output
				of_action_enqueue* action = new of_action_enqueue();
				action->port = port;
				action->queue_id = queue_id;
				action_to_add.reset(action);
				actions.push_back(std::move(action_to_add));
			}

		} else if (kv[index].first == "set_vlan") {
			uint16_t id;
			if (!string_utils::string_to_uint16(kv[index].second, &id)) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse set_vlan id [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_vlan_id* action = new of_action_set_vlan_id();
			action->vlan_id = id;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "set_vlan_pcp") {
			uint16_t pcp;
			if (!string_utils::string_to_uint16(kv[index].second, & pcp) || pcp > 0xff) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse set_vlan_pcp pcp [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_vlan_pcp* action = new of_action_set_vlan_pcp();
			action->vlan_pcp = (uint8_t) pcp;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));
		
		} else if(kv[index].first == "strip_vlan") {
			uint16_t value;
			if (!string_utils::string_to_uint16(kv[index].second, &value) || value > 1) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse strip_vlan value [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}
			if (value == 1) {
				actions.push_back(std::move(std::unique_ptr<of_action>(new of_action_strip_vlan())));
			}

		} else if (kv[index].first == "set_dl_src") {
			mac_address dl_src;
			dl_src = kv[index].second;
			if (kv[index].second != dl_src.to_string()) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse set_dl_src mac [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_mac_address* action = new of_action_set_mac_address();
			action->set_src = true;
			action->new_addr = dl_src;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "set_dl_dest") {
			mac_address dl_dest;
			dl_dest = kv[index].second;
			if (kv[index].second != dl_dest.to_string()) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse set_dl_dest mac [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_mac_address *action = new of_action_set_mac_address();
			action->set_src = false;
			action->new_addr = dl_dest;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "set_nw_src") {
			ip_address nw_src;
			nw_src = kv[index].second;
			if (kv[index].second != nw_src.to_string()) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse set_nw_src ip [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_ip_address* action = new of_action_set_ip_address();
			action->set_src = true;
			action->new_addr = nw_src;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "set_nw_dest") {
			ip_address nw_dest;
			nw_dest = kv[index].second;
			if (kv[index].second != nw_dest.to_string()) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse set_nw_dest ip [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_ip_address* action = new of_action_set_ip_address();
			action->set_src = false;
			action->new_addr = nw_dest;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "set_ip_tos") {
			uint16_t tos;
			if (!string_utils::string_to_uint16(kv[index].second, &tos) || tos > 127) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse ip_tos tos [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_ip_type_of_service* action = new of_action_set_ip_type_of_service();
			action->type_of_service = tos;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "set_src_port") {
			uint16_t port;
			if (!string_utils::string_to_uint16(kv[index].second, &port)) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse src_port [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_tcpudp_port* action = new of_action_set_tcpudp_port();
			action->set_src_port = true;
			action->port = port;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "set_dest_port") {
			uint16_t port;
			if (!string_utils::string_to_uint16(kv[index].second, &port)) {
				output::log(output::loglevel::WARNING, "openflow_action_list::from_string() cannot parse dest_port [%s]; ignoring.\n", kv[index].second.c_str());
				result = false;
				continue;
			}

			// synthesize output
			of_action_set_tcpudp_port* action = new of_action_set_tcpudp_port();
			action->set_src_port = false;
			action->port = port;
			action_to_add.reset(action);
			actions.push_back(std::move(action_to_add));

		} else if (kv[index].first == "vendor") {
			output::log(output::loglevel::WARNING, "openflow_action_list::from_string() vendor action not supported; ignoring.\n");
			result = false;
		} else {
			output::log(output::loglevel::WARNING, "openflow_action_list::from_string() unknown command [%s]; ignoring.\n", kv[index].second.c_str());
			result = false;
		}
		
	}

	return result;
}

// adds an action from an of_action
void openflow_action_list::add_action(const of_action& action) {

	// make sure there are no duplicates
	for (const auto& orig_action_ptr : actions) {
		if (*orig_action_ptr == action) {
			return;
		}
	}

	std::unique_ptr<of_action> action_copy(of_action::clone(&action));
	actions.push_back(std::move(action_copy));
}

// returns a copy of the vector of actions
std::vector<std::unique_ptr<of_action>> openflow_action_list::get_actions() const {
	std::vector<std::unique_ptr<of_action>> result;

	for (const auto& orig_action_ptr : actions) {
		result.push_back(std::unique_ptr<of_action>(of_action::clone(orig_action_ptr.get())));

	}

	return result;
}

// serializes the action list
uint32_t openflow_action_list::serialize(autobuf& dest) const {

	if (actions.empty()) {
		dest.clear();
		return 0;
	}

	uint32_t size_required = get_serialization_size();
	uint32_t offset = 0;
	dest.create_empty_buffer(size_required, false);

	for (const auto& action : actions) {
		uint32_t deserialization_size = action->get_deserialization_size();
		autobuf current_buf;
		current_buf.inherit_shared(dest.ptr_offset_mutable(offset),
			deserialization_size, deserialization_size);
//		autobuf current_buf(dest.ptr_offset(offset), deserialization_size, deserialization_size, false);
		action->serialize(current_buf);
		offset += deserialization_size;
	}

	return size_required;
}

// returns the size required for serialization
uint32_t openflow_action_list::get_serialization_size() const {
	uint32_t result = 0;
	for (const auto& action : actions) {
		result += action->get_deserialization_size();
	}
	return result;
}

// deserializes an incoming action list
bool openflow_action_list::deserialize(const autobuf& input) {
	clear();
	if (input.size() == 0) {
		return true;
	} else if (input.size() < sizeof(struct ofp_action_header)) {
		return false;
	}

	uint32_t offset = 0;
	while (offset+sizeof(struct ofp_action_header) <= input.size()) {
		autobuf type_buf;
		type_buf.inherit_read_only(input.ptr_offset_const(offset), sizeof(struct ofp_action_header));
		of_action::action_type type = of_action::infer_type(type_buf);
//		of_action::action_type type = of_action::infer_type(input.ptr_offset(offset));
		std::unique_ptr<of_action> current_action;
		autobuf current_autobuf;
		current_autobuf.inherit_read_only(input.ptr_offset_const(offset), input.size()-offset);
		bool status;

		switch (type) {
		case of_action::action_type::OUTPUT_TO_PORT:
			current_action.reset(new of_action_output_to_port());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::ENQUEUE:
			current_action.reset(new of_action_enqueue());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::SET_VLAN_ID:
			current_action.reset(new of_action_set_vlan_id());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::SET_VLAN_PCP:
			current_action.reset(new of_action_set_vlan_pcp());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::STRIP_VLAN:
			current_action.reset(new of_action_strip_vlan());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::SET_DL_ADDR:
			current_action.reset(new of_action_set_mac_address());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::SET_NW_ADDR:
			current_action.reset(new of_action_set_ip_address());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::SET_IP_TOS:
			current_action.reset(new of_action_set_ip_type_of_service());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::SET_TCPUDP_PORT:
			current_action.reset(new of_action_set_tcpudp_port());
			status = current_action->deserialize(current_autobuf);
			break;

		case of_action::action_type::VENDOR:
			current_action.reset(new of_action_vendor());
			status = current_action->deserialize(current_autobuf);
			break;

		default:
			goto fail;
		}

		if (!status) {
			goto fail;
		}

		offset += current_action->get_deserialization_size();
		actions.push_back(std::move(current_action));
	}

	return true;

fail:

	actions.clear();
	return false;
}

