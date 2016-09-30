#include "ip_port.h"

// default constructor
ip_port::ip_port() {
	clear();
}

// shortcut constructor
ip_port::ip_port(const string& input) {
	validate_ip_port(input, this);
}

// shortcut constructor
ip_port::ip_port(const string& input_address, uint16_t _port) {
	set(input_address, _port);
}

// shortcut constructor
ip_port::ip_port(const ip_address& input_address, uint16_t _port) {
	set(input_address, _port);
}

// shortcut constructor
ip_port::ip_port(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4, uint16_t port) {
	set(octet1, octet2, octet3, octet4, port);
}

// clears the ip port tuple
void ip_port::clear() {
	address.clear();
	port = 0;
}

// sets the ip via an ip_address type and port tuple via uint16
bool ip_port::set(const ip_address& input_address, uint16_t _port) {
	if (!address.set(input_address)) {
		clear();
		return false;
	}
	port = _port;

	return true;
}

// sets the ip via a string and port tuple via uint16 (note: the string must be a pure IP
// address with no port specifier)
bool ip_port::set(const string& input_address, uint16_t _port) {
	if (!address.set(input_address)) {
		clear();
		return false;
	}
	port = _port;
	return true;
}

// sets the ip via a composite string of address:port
bool ip_port::set(const string& input) {
	return validate_ip_port(input, this);
}

// sets the ip and port
bool ip_port::set(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4, uint16_t _port) {
	address.set(octet1, octet2, octet3, octet4);
	port = _port;
	return true;
}

// sets the address
bool ip_port::set_address(const ip_address& input_address) {
	return address.set(input_address);
}

// sets the address
bool ip_port::set_address(const string& input_address) {
	return address.set(input_address);
}

// sets the address
bool ip_port::set_address(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4) {
	return address.set(octet1, octet2, octet3, octet4);
}

// sets the port
bool ip_port::set_port(uint16_t _port) {
	port = _port;
	return true;
}

// gets the port
uint16_t ip_port::get_port() const {
	return port;
}

// returns the ip address object
ip_address ip_port::get_address() const {
	return address;
}

// comparator operator
bool ip_port::operator==(const ip_port& input) const {
	return address == input.address && port == input.port;
}

// comparator operator
bool ip_port::operator!=(const ip_port& input) const {
	return !(*this == input);
}

// checks if the ip port is ready for use
bool ip_port::is_nil() const {
	return address.is_nil() || port == 0;
}

// serializes the ip_port tuple
autobuf ip_port::serialize() const {
	ip_port_hdr_t hdr;
	address.get_address_network_order(hdr.address);
	(*(uint16_t*) hdr.port) = htons(port);
	autobuf ret;
	ret.set_content(&hdr, sizeof(hdr));
	return ret;
}

// deserializes the ip_port tuple
bool ip_port::deserialize(const autobuf& input) {
	clear();
	if (input.size() < 6) {
		return false;
	}

	const ip_port_hdr_t* ptr = (const ip_port_hdr_t*) input.get_content_ptr();
	address.set_from_network_buffer(ptr->address);
	port = ntohs(*((const uint16_t*)(ptr->port)));

	return true;
}

// gets the composite address:port string
string ip_port::to_string() const {
	char buf[16];
	sprintf(buf, ":%u", port);
	return address.to_string()+string(buf);
}

// validates an ip_port tuple
bool ip_port::validate_ip_port(const string& input, ip_port* dest) {

	char buf[32];
	int values[5];
	
	if ((sscanf(input.c_str(), "%d.%d.%d.%d:%d", &values[0], &values[1],
		&values[2], &values[3], &values[4]) != 5) || 
		values[0] < 0 || values[0] > 255 ||
		values[1] < 0 || values[1] > 255 ||
		values[2] < 0 || values[2] > 255 ||
		values[3] < 0 || values[3] > 255 ||
		values[4] < 0 || values[4] > 65535) {
		
		goto fail;
	} 
	
	// one more sanity check to make sure there were no trailing contents
	sprintf(buf, "%d.%d.%d.%d:%d", values[0], values[1], values[2], values[3], values[4]);
	if (strcmp(input.c_str(), buf) != 0) {
		goto fail;
	}

	// success. modify dest if required
	if (dest != nullptr) {
		dest->set((uint8_t) values[0], (uint8_t) values[1], (uint8_t) values[2], (uint8_t) values[3], (uint16_t) values[4]);
	}

	return true;

fail:
	if (dest != nullptr) {
		dest->clear();
	}
	return false;
}

