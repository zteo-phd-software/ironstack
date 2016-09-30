#include "of_actions_supported.h"

// default constructor
of_actions_supported::of_actions_supported()
{
	clear();
}

// shortcut constructor
of_actions_supported::of_actions_supported(uint32_t raw_value)
{
	set(raw_value);
}

// clears the object
void of_actions_supported::clear()
{
	output_to_switch_port = false;
	set_vlan_vid = false;
	set_vlan_pcp = false;
	strip_vlan_hdr = false;
	set_ethernet_src = false;
	set_ethernet_dest = false;
	set_ip_src = false;
	set_ip_dest = false;
	set_ip_type_of_service = false;
	set_tcpudp_src_port = false;
	set_tcpudp_dest_port = false;
	enqueue = false;
	vendor = false;
}

// sets the state based on the input raw value bitmask
void of_actions_supported::set(uint32_t raw_value)
{
	output_to_switch_port = (raw_value & (uint32_t) OFPAT_OUTPUT) ? true : false;
	set_vlan_vid = (raw_value & (uint32_t) OFPAT_SET_VLAN_VID) ? true : false;
	set_vlan_pcp = (raw_value & (uint32_t) OFPAT_SET_VLAN_PCP) ? true : false;
	strip_vlan_hdr = (raw_value & (uint32_t) OFPAT_STRIP_VLAN) ? true : false;
	set_ethernet_src = (raw_value & (uint32_t) OFPAT_SET_DL_SRC) ? true : false;
	set_ethernet_dest = (raw_value & (uint32_t) OFPAT_SET_DL_DST) ? true : false;
	set_ip_src = (raw_value & (uint32_t) OFPAT_SET_NW_SRC) ? true : false;
	set_ip_dest = (raw_value & (uint32_t) OFPAT_SET_NW_DST) ? true : false;
	set_ip_type_of_service = (raw_value & (uint32_t) OFPAT_SET_NW_TOS) ? true : false;
	set_tcpudp_src_port = (raw_value & (uint32_t) OFPAT_SET_TP_SRC) ? true : false;
	set_tcpudp_dest_port = (raw_value & (uint32_t) OFPAT_SET_TP_DST) ? true : false;
	enqueue = (raw_value & (uint32_t) OFPAT_ENQUEUE) ? true : false;
	vendor = (raw_value & (uint32_t) OFPAT_VENDOR) ? true : false;
}

// derives the raw value from the switches
uint32_t of_actions_supported::get() const
{
	uint32_t result = 0;
	result |= (output_to_switch_port ? (uint32_t) OFPAT_OUTPUT : 0);
	result |= (set_vlan_vid ? (uint32_t) OFPAT_SET_VLAN_VID : 0);
	result |= (set_vlan_pcp ? (uint32_t) OFPAT_SET_VLAN_PCP : 0);
	result |= (strip_vlan_hdr ? (uint32_t) OFPAT_STRIP_VLAN : 0);
	result |= (set_ethernet_src ? (uint32_t) OFPAT_SET_DL_SRC : 0);
	result |= (set_ethernet_dest ? (uint32_t) OFPAT_SET_DL_DST : 0);
	result |= (set_ip_src ? (uint32_t) OFPAT_SET_NW_SRC : 0);
	result |= (set_ip_dest ? (uint32_t) OFPAT_SET_NW_DST : 0);
	result |= (set_ip_type_of_service ? (uint32_t) OFPAT_SET_NW_TOS : 0);
	result |= (set_tcpudp_src_port ? (uint32_t) OFPAT_SET_TP_SRC : 0);
	result |= (set_tcpudp_dest_port ? (uint32_t) OFPAT_SET_TP_DST : 0);
	result |= (enqueue ? (uint32_t) OFPAT_ENQUEUE : 0);
	result |= (vendor ? (uint32_t) OFPAT_VENDOR : 0);

	return result;
}

// generates a debug message about the actions supported
std::string of_actions_supported::to_string() const
{
	std::string result = "actions supported:\n";

	if (output_to_switch_port)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "output to port";

	if (set_vlan_vid)
		result += "       [X] ";
	else
		result += "       [ ] ";
	result += "set vlan vid\n";

	if (set_vlan_pcp)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "set vlan pcp";

	if (strip_vlan_hdr)
		result += "         [X] ";
	else
		result += "         [ ] ";
	result += "strip vlan hdr\n";

	if (set_ethernet_src)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "set eth src";

	if (set_ethernet_dest)
		result += "          [X] ";
	else
		result += "          [ ] ";
	result += "set eth dest\n";

	if (set_ip_src)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "set IP src";

	if (set_ip_dest)
		result += "           [X] ";
	else
		result += "           [ ] ";
	result += "set IP dest\n";

	if (set_ip_type_of_service)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "set IP TOS";

	if (enqueue)
		result += "           [X] ";
	else
		result += "           [ ] ";
	result += "enqueue\n";

	if (set_tcpudp_src_port)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "set TCPUDP src port";

	if (set_tcpudp_dest_port)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "set TCPUDP dest port\n";

	if (vendor)
		result += "  [X] ";
	else
		result += "  [ ] ";
	result += "vendor-specific";

	return result;
}
