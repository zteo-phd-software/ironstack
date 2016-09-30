#include "dell_s48xx_l2_table.h"

// dell s48xx L2 tables must be of the following format:
// criteria: dl_vlan = xx AND dl_dest = xx.
// action: output_port = xx ONLY (only one output port and one action)
bool dell_s48xx_l2_table::check_table_fit(const of_match& criteria,
	const openflow_action_list& action_list) const {

	if (criteria.wildcard_in_port &&
		criteria.wildcard_ethernet_src &&
		!criteria.wildcard_ethernet_dest &&
		!criteria.ethernet_dest.is_broadcast() && 
		!criteria.wildcard_vlan_id &&
		criteria.wildcard_vlan_pcp &&
		criteria.wildcard_ethernet_frame_type &&
		criteria.wildcard_ip_type_of_service &&
		criteria.wildcard_ip_protocol &&
		criteria.wildcard_ip_src_lsb_count == 32 &&
		criteria.wildcard_ip_dest_lsb_count == 32 &&
		criteria.wildcard_tcpudp_src_port &&
		criteria.wildcard_tcpudp_dest_port) {

		vector<unique_ptr<of_action>> actions = action_list.get_actions();
		if (actions.size() != 1 ||
			actions[0]->get_action_type() != of_action::action_type::OUTPUT_TO_PORT) {
			return false;
		} else {
			return true;
		}
	}

	return false;
}

// returns the name of the table
string dell_s48xx_l2_table::get_table_name() const {
	return string("dell_s48xx L2 table");
}
