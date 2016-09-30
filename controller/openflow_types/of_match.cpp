#include "of_match.h"
#include "../gui/output.h"

// constructor
of_match::of_match() {
	clear();
}

// constructor
of_match::of_match(const string& request) {
	if (!from_string(request)) {
		clear();
	}
}

// checks equality of objects
bool of_match::operator==(const of_match& other) const {

	// in port
	if (wildcard_in_port != other.wildcard_in_port
		|| (!wildcard_in_port && in_port != other.in_port)) {
		return false;
	}

	// dl src
	if (wildcard_ethernet_src != other.wildcard_ethernet_src
		|| (!wildcard_ethernet_src && ethernet_src != other.ethernet_src)) {
		return false;
	}

	// dl dst
	if (wildcard_ethernet_dest != other.wildcard_ethernet_dest
		|| (!wildcard_ethernet_dest && ethernet_dest != other.ethernet_dest)) {
		return false;
	}

	// vlan id
	if (wildcard_vlan_id != other.wildcard_vlan_id
		|| (!wildcard_vlan_id && vlan_id != other.vlan_id)) {
		return false;
	}

	// vlan pcp
	if (wildcard_vlan_pcp != other.wildcard_vlan_pcp
		|| (!wildcard_vlan_pcp && vlan_pcp != other.vlan_pcp)) {
		return false;
	}

	// ethernet frame type
	if (wildcard_ethernet_frame_type != other.wildcard_ethernet_frame_type
		|| (!wildcard_ethernet_frame_type && ethernet_frame_type != other.ethernet_frame_type)) {
		return false;
	}

	// ip tos
	if (wildcard_ip_type_of_service != other.wildcard_ip_type_of_service
		|| (!wildcard_ip_type_of_service && ip_type_of_service != other.ip_type_of_service)) {
		return false;
	}

	// ip protocol
	if (wildcard_ip_protocol != other.wildcard_ip_protocol
		|| (!wildcard_ip_protocol && ip_protocol != other.ip_protocol)) {
		return false;
	}

	// ip src address
	if (wildcard_ip_src_lsb_count != other.wildcard_ip_src_lsb_count 
		|| (wildcard_ip_src_lsb_count < 32 && ip_src_address != other.ip_src_address)) {
		return false;
	}

	// ip dest address
	if (wildcard_ip_dest_lsb_count != other.wildcard_ip_dest_lsb_count
		|| (wildcard_ip_dest_lsb_count < 32 && ip_dest_address != other.ip_dest_address)) {
		return false;
	}

	// tcp src port
	if (wildcard_tcpudp_src_port != other.wildcard_tcpudp_src_port
		|| (!wildcard_tcpudp_src_port && tcpudp_src_port != other.tcpudp_src_port)) {
		return false;
	}

	// tcp dest port
	if (wildcard_tcpudp_dest_port != other.wildcard_tcpudp_dest_port
		|| (!wildcard_tcpudp_dest_port && tcpudp_dest_port != other.tcpudp_dest_port)) {
		return false;
	}

	// all checks passed
	return true;

	/*
	autobuf buf1;
	autobuf buf2;

	this->serialize(buf1);
	other.serialize(buf2);

	if (buf1 == buf2) {
		return true;
	}
	return false;
	*/
}

bool of_match::operator!=(const of_match& other) const {
	return !(*this == other);
}

// check if this is subset of other
bool of_match::operator<(const of_match& other) const {

	// in port
	if (!other.wildcard_in_port && (wildcard_in_port || (in_port != other.in_port))) {
		return false;
	}

	// eth src
	if (!other.wildcard_ethernet_src && (wildcard_ethernet_src || (ethernet_src != other.ethernet_src))) {
		return false;
	}

	// eth dest
	if (!other.wildcard_ethernet_dest && (wildcard_ethernet_dest || (ethernet_dest != other.ethernet_dest))) {
		return false;
	}

	// check vlan id 
	if (!other.wildcard_vlan_id && (wildcard_vlan_id || (vlan_id != other.vlan_id))) {
		return false;
	}

	// check vlan pcp
	if (!other.wildcard_vlan_pcp && (wildcard_vlan_pcp || (vlan_pcp != other.vlan_pcp))) {
		return false;
	}

	// check ethernet_frame_type 
	if (!other.wildcard_ethernet_frame_type && (wildcard_ethernet_frame_type || (ethernet_frame_type != other.ethernet_frame_type))) {
		return false;
	}

	// check ip tos
	if (!other.wildcard_ip_type_of_service && (wildcard_ip_type_of_service || (ip_type_of_service != other.ip_type_of_service))) {
		return false;
	}

	// check ip protocol 
	if (!other.wildcard_ip_protocol && (wildcard_ip_protocol || (ip_protocol != other.ip_protocol))) {
		return false;
	}

	// check ip_src wildcard
	char addr1[4];
	char addr2[4];
	uint8_t prefix_size;
	uint8_t counter;
	if (other.wildcard_ip_src_lsb_count >= wildcard_ip_src_lsb_count) {
		ip_src_address.get_address_network_order(addr1);
		other.ip_src_address.get_address_network_order(addr2);

		// check prefix for ip src
		prefix_size = 32-wildcard_ip_src_lsb_count;
		for (counter = 0; counter < prefix_size; ++counter) {
			if ((addr1[counter/8] & (1 << (counter%8))) != (addr2[counter/8] & (1 << (counter%8)))) {
				return false;
			}
		}
	}

	// check ip dest wildcard
	if (other.wildcard_ip_dest_lsb_count >= wildcard_ip_dest_lsb_count) {
		ip_dest_address.get_address_network_order(addr1);
		other.ip_dest_address.get_address_network_order(addr2);

		// check prefix for ip src
		prefix_size = 32-wildcard_ip_dest_lsb_count;
		for (counter = 0; counter < prefix_size; ++counter) {
			if ((addr1[counter/8] & (1 << (counter%8))) != (addr2[counter/8] & (1 << (counter%8)))) {
				return false;
			}
		}
	}

	// check tcpudp_src_port
	if (!other.wildcard_tcpudp_src_port && (wildcard_tcpudp_src_port || (tcpudp_src_port != other.tcpudp_src_port))) {
		return false;
	}

	// check tcpudp dest_port
	if (!other.wildcard_tcpudp_dest_port && (wildcard_tcpudp_dest_port || (tcpudp_dest_port != other.tcpudp_dest_port))) {
		return false;
	}

	// all subset checks passed
	return true;
}

// clears the object
void of_match::clear(){
	wildcard_none();
	in_port = 0;
	ethernet_src.clear();
	ethernet_dest.clear();
	vlan_id = 0;
	vlan_pcp = 0;
	ethernet_frame_type = 0;
	ip_type_of_service = 0;
	ip_protocol = 0;
	ip_src_address.clear();
	ip_dest_address.clear();
	tcpudp_src_port = 0;
	tcpudp_dest_port = 0;
}

// sets the wildcard field for all tuples
void of_match::wildcard_all() {
	wildcard_in_port = true;
	wildcard_ethernet_src = true;
	wildcard_ethernet_dest = true;
	wildcard_vlan_id = true;
	wildcard_vlan_pcp = true;
	wildcard_ethernet_frame_type = true;
	wildcard_ip_type_of_service = true;
	wildcard_ip_protocol = true;
	wildcard_ip_src_lsb_count = 32;
	wildcard_ip_dest_lsb_count = 32;
	wildcard_tcpudp_src_port = true;
	wildcard_tcpudp_dest_port = true;
}

// clears the wildcard field for all tuples
void of_match::wildcard_none() {
	wildcard_in_port = false;
	wildcard_ethernet_src = false;
	wildcard_ethernet_dest = false;
	wildcard_vlan_id = false;
	wildcard_vlan_pcp = false;
	wildcard_ethernet_frame_type = false;
	wildcard_ip_type_of_service = false;
	wildcard_ip_protocol = false;
	wildcard_ip_src_lsb_count = 0;
	wildcard_ip_dest_lsb_count = 0;
	wildcard_tcpudp_src_port = false;
	wildcard_tcpudp_dest_port = false;
}

// generates a ruleset from a given input string
bool of_match::from_string(const string& request) {
	
	wildcard_all();
	bool result = true;

	std::vector<std::pair<string, string>> kv_pairs = string_utils::generate_key_value_pair(request);
	for (const auto& kv : kv_pairs) {
		if (kv.first == "dl_src" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_ethernet_src = false;
			ethernet_src = kv.second;
			if (ethernet_src.to_string() != kv.second) {
				wildcard_ethernet_src = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse dl_src address [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			}

		} else if (kv.first == "dl_dest" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_ethernet_dest = false;
			ethernet_dest = kv.second;
			if (ethernet_dest.to_string() != kv.second) {
				wildcard_ethernet_dest = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse dl_dest address [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			}

		} else if (kv.first == "in_port" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_in_port = false;
			if (!string_utils::string_to_uint16(kv.second, &in_port)) {
				wildcard_in_port = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse in_port [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			}

		} else if (kv.first == "vlan_id" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_vlan_id = false;
			if (!string_utils::string_to_uint16(kv.second, &vlan_id)) {
				wildcard_vlan_id = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse vlan_id [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			}

		} else if (kv.first == "vlan_pcp" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_vlan_pcp = false;
			uint16_t val;
			if (!string_utils::string_to_uint16(kv.second, &val) || val > 0xff) {
				wildcard_vlan_pcp = true;
				output::log(output::loglevel::WARNING, "of_match:from_string() cannot parse vlan_pcp [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			} else {
				vlan_pcp = (uint8_t) val;
			}

		} else if (kv.first == "ethertype" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_ethernet_frame_type = false;
			if (!string_utils::string_to_uint16(kv.second, &ethernet_frame_type)) {
				wildcard_ethernet_frame_type = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse ethertype [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			}

		} else if (kv.first == "ip_tos" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_ip_type_of_service = false;
			uint16_t val;
			if (!string_utils::string_to_uint16(kv.second, &val) || val > 0xff) {
				wildcard_ip_type_of_service = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse ip_tos [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			} else {
				ip_type_of_service = (uint8_t) val;
			}

		} else if (kv.first == "ip_proto" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_ip_protocol = false;
			uint16_t val;
			if (!string_utils::string_to_uint16(kv.second, &val) || val > 0xff) {
				wildcard_ip_protocol = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse ip_proto [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			} else {
				ip_protocol = (uint8_t) val;
			}

		} else if (kv.first == "nw_src_lsb_wildcard" && kv.second.size() > 0) {
			uint16_t val;
			if (!string_utils::string_to_uint16(kv.second, &val) || val > 32) {
				wildcard_ip_src_lsb_count = 32;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse nw_src_lsb_wildcard [%s]; setting to 32.\n", kv.second.c_str());
				result = false;
			} else {
				wildcard_ip_src_lsb_count = (uint8_t) val;
			}

		} else if (kv.first == "nw_dest_lsb_wildcard" && kv.second.size() > 0) {
			uint16_t val;
			if (!string_utils::string_to_uint16(kv.second, &val) || val > 32) {
				wildcard_ip_dest_lsb_count = 32;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse nw_dest_lsb_wildcard [%s]; setting to 32.\n", kv.second.c_str());
				result = false;
			} else {
				wildcard_ip_dest_lsb_count = (uint8_t) val;
			}

		} else if (kv.first == "nw_src" && kv.second.size() > 0 && kv.second != "*") {
			ip_src_address.set(kv.second);
			if (ip_src_address.to_string() != kv.second) {
				wildcard_ip_src_lsb_count = 32;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse nw_src [%s]; setting nw_src_lsb_wildcard to 32.\n", kv.second.c_str());
				result = false;
			}

		} else if (kv.first == "nw_dest" && kv.second.size() > 0 && kv.second != "*") {
			ip_dest_address.set(kv.second);
			if (ip_dest_address.to_string() != kv.second) {
				wildcard_ip_dest_lsb_count = 32;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse nw_dest [%s]; setting nw_dest_lsb_wildcard to 32.\n", kv.second.c_str()); 
				result = false;
			}

		} else if (kv.first == "src_port" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_tcpudp_src_port = false;
			if (!string_utils::string_to_uint16(kv.second, &tcpudp_src_port)) {
				wildcard_tcpudp_src_port = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse src_port [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			}

		} else if (kv.first == "dest_port" && kv.second.size() > 0 && kv.second != "*") {
			wildcard_tcpudp_dest_port = false;
			if (!string_utils::string_to_uint16(kv.second, &tcpudp_dest_port)) {
				wildcard_tcpudp_dest_port = true;
				output::log(output::loglevel::WARNING, "of_match::from_string() cannot parse dest_port [%s]; setting to wildcard.\n", kv.second.c_str());
				result = false;
			}
		} else {
			output::log(output::loglevel::WARNING, "of_match::from_string() unknown setting [%s] = [%s]; ignored.\n", kv.first.c_str(), kv.second.c_str());
			result = false;
		}
	}

	return result;
}

// generates a readable form of the object
string of_match::to_string() const {

	string result;
	char buf[32];
	bool specified = false;	// if all fields are wildcarded, this is false

	if (!wildcard_in_port) {
		sprintf(buf, "in_port=%hu ", in_port);
		result += string(buf);
		specified = true;
	}

	if (!wildcard_ethernet_src) {
		result += string("dl_src=") + ethernet_src.to_string() + string(" ");
		specified = true;
	}

	if (!wildcard_ethernet_dest) {
		result += string("dl_dest=") + ethernet_dest.to_string() + string(" ");
		specified = true;
	}

	if (!wildcard_vlan_id) {
		sprintf(buf, "vlan_id=%hu ", vlan_id);
		result += string(buf);
		specified = true;
	}

	if (!wildcard_vlan_pcp) {
		sprintf(buf, "vlan_pcp=%hu ", vlan_pcp);
		result += string(buf);
		specified = true;
	}

	if (!wildcard_ethernet_frame_type) {
		sprintf(buf, "eth_type=%hu ", ethernet_frame_type);
		result += string(buf);
		specified = true;
	}

	if (!wildcard_ip_type_of_service) {
		sprintf(buf, "ip_TOS=%hu ", ip_type_of_service);
		result += string(buf);
		specified = true;
	}

	if (!wildcard_ip_protocol) {
		sprintf(buf, "ip_proto=%hu ", ip_protocol);
		result += string(buf);
		specified = true;
	}

	if (wildcard_ip_src_lsb_count < 32) {
		sprintf(buf, "nw_src=%s/%hu ", ip_src_address.to_string().c_str(), wildcard_ip_src_lsb_count);
		result += string(buf);
		specified = true;
	}

	if (wildcard_ip_dest_lsb_count < 32) {
		sprintf(buf, "nw_dest=%s/%u ", ip_dest_address.to_string().c_str(), wildcard_ip_dest_lsb_count);
		result += string(buf);
		specified = true;
	}

	if (!wildcard_tcpudp_src_port) {
		sprintf(buf, "nw_src_port=%hu ", tcpudp_src_port);
		result += string(buf);
		specified = true;
	}

	if (!wildcard_tcpudp_dest_port) {
		sprintf(buf, "nw_dest_port=%hu ", tcpudp_dest_port);
		result += string(buf);
		specified = true;
	}

	if (!specified) {
		result += string("all fields wildcarded.");
	}

	return result;
}

// serializes the object into a struct ofp_match
uint32_t of_match::serialize(autobuf& dest) const {
	dest.create_empty_buffer(sizeof(struct ofp_match), false);
	struct ofp_match* result = (struct ofp_match*) dest.get_content_ptr_mutable();
	result->pad1[0] = 0;
	*((uint16_t*) result->pad2) = 0;

	uint32_t wildcard = 0;
	uint32_t lsbs_to_wildcard;

	// generate wildcard field
	wildcard |= wildcard_in_port ? (uint32_t) OFPFW_IN_PORT : 0;
	wildcard |= wildcard_ethernet_src ? (uint32_t) OFPFW_DL_SRC : 0;
	wildcard |= wildcard_ethernet_dest ? (uint32_t) OFPFW_DL_DST : 0;
	wildcard |= wildcard_vlan_id ? (uint32_t) OFPFW_DL_VLAN : 0;
	wildcard |= wildcard_vlan_pcp ? (uint32_t) OFPFW_DL_VLAN_PCP : 0;
	wildcard |= wildcard_ethernet_frame_type ? (uint32_t) OFPFW_DL_TYPE : 0;
	wildcard |= wildcard_ip_type_of_service ? (uint32_t) OFPFW_NW_TOS : 0;
	wildcard |= wildcard_ip_protocol ? (uint32_t) OFPFW_NW_PROTO : 0;

	lsbs_to_wildcard = wildcard_ip_src_lsb_count > 32 ? 32 : wildcard_ip_src_lsb_count;
	wildcard |= ((lsbs_to_wildcard << OFPFW_NW_SRC_SHIFT) & OFPFW_NW_SRC_MASK);
	lsbs_to_wildcard = wildcard_ip_dest_lsb_count > 32 ? 32 : wildcard_ip_dest_lsb_count;
	wildcard |= ((lsbs_to_wildcard << OFPFW_NW_DST_SHIFT) & OFPFW_NW_DST_MASK);

	wildcard |= wildcard_tcpudp_src_port ? (uint32_t) OFPFW_TP_SRC : 0;
	wildcard |= wildcard_tcpudp_dest_port ? (uint32_t) OFPFW_TP_DST : 0;

	// fill in the struct
	result->wildcards = htonl(wildcard);
	result->in_port = htons(in_port);
	ethernet_src.get(result->dl_src);
	ethernet_dest.get(result->dl_dst);
	result->dl_vlan = htons(vlan_id);
	result->dl_vlan_pcp = vlan_pcp;
	result->dl_type = htons(ethernet_frame_type);
	result->nw_tos = ip_type_of_service;
	result->nw_proto = ip_protocol;

	ip_src_address.get_address_network_order(&result->nw_src);
	ip_dest_address.get_address_network_order(&result->nw_dst);
	result->tp_src = htons(tcpudp_src_port);
	result->tp_dst = htons(tcpudp_dest_port);

	return sizeof(struct ofp_match);
}

// deserializes an input struct of_match and populates this object
bool of_match::deserialize(const autobuf& input) {
	const struct ofp_match* header = (const struct ofp_match*) input.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	clear();
	if (input.size() < sizeof(struct ofp_match))
		return false;
	#endif

//	memcpy(&header, input.get_content_ptr(), sizeof(struct ofp_match));

	in_port = ntohs(header->in_port);
	ethernet_src.set_from_network_buffer(header->dl_src);
	ethernet_dest.set_from_network_buffer(header->dl_dst);
	vlan_id = ntohs(header->dl_vlan);
	vlan_pcp = header->dl_vlan_pcp;
	ethernet_frame_type = ntohs(header->dl_type);
	ip_type_of_service = header->nw_tos;
	ip_protocol = header->nw_proto;

	ip_src_address.set_from_network_buffer(&header->nw_src);
	ip_dest_address.set_from_network_buffer(&header->nw_dst);
	tcpudp_src_port = ntohs(header->tp_src);
	tcpudp_dest_port = ntohs(header->tp_dst);

	uint32_t wildcards = ntohl(header->wildcards);

	// setup wildcards
	wildcard_in_port = (wildcards & (uint32_t) OFPFW_IN_PORT) > 0;
	wildcard_ethernet_src = (wildcards & (uint32_t) OFPFW_DL_SRC) > 0;
	wildcard_ethernet_dest = (wildcards & (uint32_t) OFPFW_DL_DST) > 0;
	wildcard_vlan_id = (wildcards & (uint32_t) OFPFW_DL_VLAN) > 0;
	wildcard_vlan_pcp = (wildcards & (uint32_t) OFPFW_DL_VLAN_PCP) > 0;
	wildcard_ethernet_frame_type = (wildcards & (uint32_t) OFPFW_DL_TYPE) > 0;
	wildcard_ip_type_of_service = (wildcards & (uint32_t) OFPFW_NW_TOS) > 0;
	wildcard_ip_protocol = (wildcards & (uint32_t) OFPFW_NW_PROTO) > 0;
	if (wildcards & (uint32_t) OFPFW_NW_SRC_ALL) {
		wildcard_ip_src_lsb_count = 32;
	} else {
		wildcard_ip_src_lsb_count = (wildcards & ((uint32_t) OFPFW_NW_SRC_MASK)) >> ((uint32_t) OFPFW_NW_SRC_BITS);
	}
	if (wildcards & (uint32_t) OFPFW_NW_DST_ALL) {
		wildcard_ip_dest_lsb_count = 32;
	} else {
		wildcard_ip_dest_lsb_count = (wildcards & ((uint32_t) OFPFW_NW_DST_MASK)) >> ((uint32_t) OFPFW_NW_DST_BITS);
	}
	wildcard_tcpudp_src_port = (wildcards & (uint32_t) OFPFW_TP_SRC) > 0;
	wildcard_tcpudp_dest_port = (wildcards & (uint32_t) OFPFW_TP_DST) > 0;

	return true;
}
