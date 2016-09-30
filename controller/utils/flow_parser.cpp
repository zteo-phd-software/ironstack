#include "flow_parser.h"
#include "../gui/output.h"

// loads flows from a file
bool flow_parser::load_flows(const string& filename) {
	flows.clear();
	autobuf_packer packer;
	if (!packer.deserialize_from_disk(filename)) {
		output::log(output::loglevel::ERROR, "flow_parser::load_flows() unable to load flows from [%s].\n", filename.c_str());
		return false;
	}

	vector<autobuf> buffers = packer.get_autobufs();
	flows.reserve(buffers.size());
	for (const auto& buf : buffers) {
		openflow_flow_description description;
		if (!description.deserialize(buf)) {
			output::log(output::loglevel::ERROR, "flow_parser::load_flows() unable to deserialize flow.\n");
			flows.clear();
			return false;
		} else {
			flows.push_back(description);
		}
	}

	return true;
}

// stores all flows locally
void flow_parser::load_flows(const vector<openflow_flow_description_and_stats>& new_flows) {
	flows.clear();
	flows.reserve(new_flows.size());
	for (const auto& flow : new_flows) {
		flows.push_back(flow.flow_description);
	}
}

// stores all flows locally
void flow_parser::load_flows(const vector<openflow_flow_description>& new_flows) {
	flows = new_flows;
}

// save flows into disk
bool flow_parser::save_flows(const string& filename) const{
	autobuf_packer packer;
	
	for (const auto& flow : flows) {
		autobuf current_flow;
		flow.serialize(current_flow);
		packer.add(current_flow);
	}

	return packer.serialize_to_disk(filename);
}

// convert flows into l2 rules; returns number of flows translated
uint32_t flow_parser::translate_to_l2(vector<openflow_flow_description>& translated_flows, vector<openflow_flow_description>& untranslated_flows) const {

	translated_flows.clear();
	untranslated_flows.clear();

	for (const auto& flow : flows) {
		of_match criteria = flow.criteria;

		// TODO -- BDF switch seems to be completely port based, why?
		if (criteria.wildcard_ethernet_src &&
			criteria.wildcard_vlan_id &&
			criteria.wildcard_vlan_pcp &&
			criteria.wildcard_ethernet_frame_type &&
			criteria.wildcard_ip_type_of_service &&
			criteria.wildcard_ip_protocol &&
			criteria.wildcard_ip_src_lsb_count == 32 &&
			criteria.wildcard_ip_dest_lsb_count == 32 &&
			criteria.wildcard_tcpudp_src_port &&
			criteria.wildcard_tcpudp_dest_port) {

			// seems to be a candidate for L2 flow
			vector<unique_ptr<of_action>> actions = flow.action_list.get_actions();

			// a candidate flow must be wildcarded in all fields except the eth dest field.
			// the action must be an output to a port.
			if (!criteria.wildcard_ethernet_dest
				&& actions.size() == 1
				&& actions[0]->get_action_type() == of_action::action_type::OUTPUT_TO_PORT) {

				// create translated flows
				openflow_flow_description description;
				description.criteria.wildcard_all();
				description.criteria.wildcard_ethernet_dest = false;
				description.criteria.ethernet_dest = criteria.ethernet_dest;
				description.action_list = flow.action_list;
				description.cookie = flow.cookie;
				description.priority = flow.priority;

				translated_flows.push_back(description);
			} else {
				untranslated_flows.push_back(flow);
			}
		} else {
			untranslated_flows.push_back(flow);
		}
	}

	return translated_flows.size();
}
