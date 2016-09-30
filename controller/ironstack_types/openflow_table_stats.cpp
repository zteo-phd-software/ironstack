#include "openflow_table_stats.h"

// constructor
openflow_table_stats::openflow_table_stats() {
	clear();
}

// clears the object
void openflow_table_stats::clear() {
	table_id = 0;
	name.clear();
	all_wildcards_supported = false;
	no_wildcards_supported = false;
	wildcard_in_port_supported = false;
	wildcard_ethernet_src_supported = false;
	wildcard_ethernet_dest_supported = false;
	wildcard_vlan_id_supported = false;
	wildcard_vlan_pcp_supported = false;
	wildcard_ethernet_frame_type_supported = false;
	wildcard_ip_type_of_service_supported = false;
	wildcard_ip_protocol_supported = false;
	wildcard_ip_src_lsb_supported = 0;
	wildcard_ip_dest_lsb_supported = 0;
	wildcard_tcpudp_src_port_supported = false;
	wildcard_tcpudp_dest_port_supported = false;
	max_entries_supported = 0;
	active_entries = 0;
	lookup_count = 0;
	matched_count = 0;
}

// generates a readable version of the object
string openflow_table_stats::to_string() const {
	string result;
	char buf[32];
	sprintf(buf, "%d", table_id);

	result = string("--- table stats ---");
	result += string("\ntable id   : ") + string(buf);
	result += string("\ntable name : ") + name;
	result += string("\nwildcard support");
	if (all_wildcards_supported) {
		result += string("\nall        : [X]");
	} else if (no_wildcards_supported) {
		result += string("\nnone       : [X]");
	} else {
		result += string("\nin port    : ") + string(wildcard_in_port_supported ? "[X]" : "[ ]");
		result += string("\neth src    : ") + string(wildcard_ethernet_src_supported ? "[X]" : "[ ]");
		result += string("\neth dest   : ") + string(wildcard_ethernet_dest_supported ? "[X]" : "[ ]");
		result += string("\nvlan id    : ") + string(wildcard_vlan_id_supported ? "[X]" : "[ ]");
		result += string("\nvlan pcp   : ") + string(wildcard_vlan_pcp_supported ? "[X]" : "[ ]");
		result += string("\nethtype    : ") + string(wildcard_ethernet_frame_type_supported ? "[X]" : "[ ]");
		result += string("\nip TOS     : ") + string(wildcard_ip_type_of_service_supported ? "[X]" : "[ ]");
		result += string("\nip proto   : ") + string(wildcard_ip_protocol_supported ? "[X]" : "[ ]");
		result += string("\ntcp srcport: ") + string(wildcard_tcpudp_src_port_supported ? "[X]" : "[ ]");
		result += string("\ntcp dstport: ") + string(wildcard_tcpudp_dest_port_supported ? "[X]" : "[ ]");
		sprintf(buf, "%d", wildcard_ip_src_lsb_supported);
		result += string("\nip src LSB : ") + string(buf) + string(" max bits supported.");
		sprintf(buf, "%d", wildcard_ip_dest_lsb_supported);
		result += string("\nip dst LSB : ") + string(buf) + string(" max bits supported.");
	}

	return result;
}

// serializes the object
uint32_t openflow_table_stats::serialize(autobuf& dest) const {
	dest.create_empty_buffer(sizeof(struct ofp_table_stats), true);

	struct ofp_table_stats* ptr = (struct ofp_table_stats*) dest.get_content_ptr_mutable();
	ptr->table_id = table_id;
	strncpy(ptr->name, name.c_str(), OFP_MAX_TABLE_NAME_LEN-1);
	uint32_t wildcards = 0;

	wildcards |= (wildcard_in_port_supported ? (uint32_t) OFPFW_IN_PORT : 0);
	wildcards |= (wildcard_ethernet_src_supported ? (uint32_t) OFPFW_DL_SRC : 0);
	wildcards |= (wildcard_ethernet_dest_supported ? (uint32_t) OFPFW_DL_DST : 0);
	wildcards |= (wildcard_vlan_id_supported ? (uint32_t) OFPFW_DL_VLAN : 0);
	wildcards |= (wildcard_vlan_pcp_supported ? (uint32_t) OFPFW_DL_VLAN_PCP : 0);
	wildcards |= (wildcard_ethernet_frame_type_supported ? (uint32_t) OFPFW_DL_TYPE : 0);
	wildcards |= (wildcard_ip_type_of_service_supported ? (uint32_t) OFPFW_NW_TOS : 0);
	wildcards |= (wildcard_ip_protocol_supported ? (uint32_t) OFPFW_NW_PROTO : 0);
	wildcards |= (((uint32_t) wildcard_ip_src_lsb_supported << OFPFW_NW_SRC_SHIFT) & OFPFW_NW_SRC_MASK);
	wildcards |= (((uint32_t) wildcard_ip_dest_lsb_supported << OFPFW_NW_DST_SHIFT) & OFPFW_NW_DST_MASK);
	wildcards |= (wildcard_tcpudp_src_port_supported ? (uint32_t) OFPFW_TP_SRC : 0);
	wildcards |= (wildcard_tcpudp_dest_port_supported ? (uint32_t) OFPFW_TP_DST : 0);

	ptr->wildcards = htonl(wildcards);
	ptr->max_entries = htonl(max_entries_supported);
	ptr->active_count = htonl(active_entries);
	ptr->lookup_count = htobe64(lookup_count);
	ptr->matched_count = htobe64(matched_count);

	return dest.size();
}

// deserializes the object
bool openflow_table_stats::deserialize(const autobuf& input) {
	clear();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (input.size() < sizeof(struct ofp_table_stats)) {
		return false;
	}
	#endif

	const struct ofp_table_stats* ptr = (const struct ofp_table_stats*)
		input.get_content_ptr();

	table_id = ptr->table_id;
	uint32_t wildcards = ntohl(ptr->wildcards);
	name = string(ptr->name);
	max_entries_supported = ntohl(ptr->max_entries);
	active_entries = ntohl(ptr->active_count);
	lookup_count = be64toh(ptr->lookup_count);
	matched_count = be64toh(ptr->matched_count);

	// generate the bitmap
	wildcard_in_port_supported = (wildcards & (uint32_t) OFPFW_IN_PORT) > 0;
	wildcard_ethernet_src_supported = (wildcards & (uint32_t) OFPFW_DL_SRC) > 0;
	wildcard_ethernet_dest_supported = (wildcards & (uint32_t) OFPFW_DL_DST) > 0;
	wildcard_vlan_id_supported = (wildcards & (uint32_t) OFPFW_DL_VLAN) > 0;
	wildcard_vlan_pcp_supported = (wildcards & (uint32_t) OFPFW_DL_VLAN_PCP) > 0;
	wildcard_ethernet_frame_type_supported = (wildcards & (uint32_t) OFPFW_DL_TYPE) > 0;
	wildcard_ip_type_of_service_supported = (wildcards & (uint32_t) OFPFW_NW_TOS) > 0;
	wildcard_ip_protocol_supported = (wildcards & (uint32_t) OFPFW_NW_PROTO) > 0;
	wildcard_ip_src_lsb_supported = (wildcards & (uint32_t) OFPFW_NW_SRC_MASK) >> OFPFW_NW_SRC_SHIFT;
	wildcard_ip_dest_lsb_supported = (wildcards & (uint32_t) OFPFW_NW_DST_MASK) >> OFPFW_NW_DST_SHIFT;
	wildcard_tcpudp_src_port_supported = (wildcards & (uint32_t) OFPFW_TP_SRC) > 0;
	wildcard_tcpudp_dest_port_supported = (wildcards & (uint32_t) OFPFW_TP_DST) > 0;

	if (wildcard_in_port_supported
		&& wildcard_ethernet_src_supported
		&& wildcard_ethernet_dest_supported
		&& wildcard_vlan_id_supported
		&& wildcard_vlan_pcp_supported
		&& wildcard_ethernet_frame_type_supported
		&& wildcard_ip_type_of_service_supported
		&& wildcard_ip_protocol_supported
		&& wildcard_ip_src_lsb_supported >= 32
		&& wildcard_ip_dest_lsb_supported >= 32
		&& wildcard_tcpudp_src_port_supported
		&& wildcard_tcpudp_dest_port_supported) {
		all_wildcards_supported = true;
	}

	if (!wildcard_in_port_supported
		&& !wildcard_ethernet_src_supported
		&& !wildcard_ethernet_dest_supported
		&& !wildcard_vlan_id_supported
		&& !wildcard_vlan_pcp_supported
		&& !wildcard_ethernet_frame_type_supported
		&& !wildcard_ip_type_of_service_supported
		&& !wildcard_ip_protocol_supported
		&& wildcard_ip_src_lsb_supported == 0
		&& wildcard_ip_dest_lsb_supported == 0
		&& !wildcard_tcpudp_src_port_supported
		&& !wildcard_tcpudp_dest_port_supported) {
		no_wildcards_supported = true;
	}

	return true;
}
