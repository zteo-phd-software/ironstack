#include "openflow_port.h"

// default constructor
openflow_port::openflow_port() {
	clear();
}

// clears the object
void openflow_port::clear() {
	port_number = 0;
	dl_addr.clear();
	config.clear();
	state.clear();
	current_features.clear();
	advertised_features.clear();
	supported_features.clear();
	peer_features.clear();
}

// shortcut constructor
openflow_port::openflow_port(const struct ofp_phy_port& input) {
	set(input);
}

// set the openflow port features directly from an openflow data structure
void openflow_port::set(const struct ofp_phy_port& input) {
	uint8_t addr_buf[OFP_ETH_ALEN];
	char name_buf[OFP_MAX_PORT_NAME_LEN+1];

	port_number = ntohs(input.port_no);

	memcpy(addr_buf, input.hw_addr, OFP_ETH_ALEN);
	dl_addr.set_from_network_buffer(addr_buf);

	memset(name_buf, 0, sizeof(name_buf));
	memcpy(name_buf, input.name, OFP_MAX_PORT_NAME_LEN);
	name = name_buf;

	config.set(ntohl(input.config));
	state.set(ntohl(input.state));
	current_features.set(ntohl(input.curr));
	advertised_features.set(ntohl(input.advertised));
	supported_features.set(ntohl(input.supported));
	peer_features.set(ntohl(input.peer));
}

// generates a debug verbose message about this physical port
string openflow_port::to_string() const {
	char buf[128];
	std::string result;
	
	snprintf(buf, sizeof(buf), "port number: %d\n", port_number);
	result += buf;
	result += "mac address        : " + dl_addr.to_string() + "\n";
	result += "name               : " + name + "\n";
	result += config.to_string() + "\n";
	result += state.to_string() + "\n";
	result += "current features   : " + current_features.to_string() + "\n";
	result += "advertised features: " + advertised_features.to_string() + "\n";
	result += "supported features : " + supported_features.to_string() + "\n";
	result += "peer features      : " + peer_features.to_string() + "\n";
 
	return result;
}
