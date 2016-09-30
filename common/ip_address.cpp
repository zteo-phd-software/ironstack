#include "ip_address.h"

// default constructor
ip_address::ip_address() {
	clear();
}

// copy constructor
ip_address::ip_address(const ip_address& input) {
	set(input);
}

// constructor with shortcut for creating address from string
ip_address::ip_address(const string& input) {
	set(input);
}

// shortcut constructor using individual octets
ip_address::ip_address(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4) {
	set(octet1, octet2, octet3, octet4);
}

// sets the address
ip_address& ip_address::operator=(const ip_address& input) {
	if (input == *this) {
		return *this;
	}

	set(input);
	return *this;
}

// sets the address
ip_address& ip_address::operator=(const string& input) {
	set(input);
	return *this;
}

// clears the ip address in one uint32_t write!
void ip_address::clear() {
	*((uint32_t*) octets) = 0;
}

// resolves an address
bool ip_address::resolve(const string& input) {

	clear();
	if (input.empty()) return false;

	struct addrinfo hints;
	struct addrinfo* result;
	int    err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	if ((err = getaddrinfo(input.c_str(), nullptr, &hints, &result)) != 0) {
		if (err != EAI_NONAME) {
			printf("ip_address::resolve() for %s failed. reason: %s\n", input.c_str(),
				gai_strerror(err));
		}
		return false;
	}

	*((uint32_t*)octets) = ((struct sockaddr_in*)(result->ai_addr))->sin_addr.s_addr;
	return true;
}

// sets the address
bool ip_address::set(const ip_address& input) {

	*((uint32_t*) octets) = *((uint32_t*) input.octets);
	return true;
}

// sets the address
bool ip_address::set(const string& input) {
	return (validate_ip(input, this));
}

// sets the address
bool ip_address::set(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4) {
	octets[0] = octet1;
	octets[1] = octet2;
	octets[2] = octet3;
	octets[3] = octet4;

	return true;
}

// sets the address direct from the network buffer
void ip_address::set_from_network_buffer(const void* buf) {
	*((uint32_t*) octets) = *((const uint32_t*) buf);
}

// sets the address from a uint32_t supplied in network endian format
void ip_address::set_from_be32(uint32_t val) {
	*((uint32_t*) octets) = val;
}

// extracts a specific octet in the address (mutable)
uint8_t& ip_address::operator[](int index) {
	if (index < 0 || index > 3) {
		printf("ip_address::operator[] error -- tried to access index %d.\n", index);
		abort();
	}
	return octets[index];
}

// extracts a specific octet in the address (not mutable)
uint8_t ip_address::operator[](int index) const {
	if (index < 0 || index > 3) {
		printf("ip_address::operator() error -- tried to access index %d.\n", index);
		abort();
	}
	return octets[index];
}

// copies the raw network value of the ip address into another buffer
void ip_address::get_address_network_order(void* buf) const {
	memcpy(buf, octets, sizeof(octets));
}

// get the raw network value of this ip address as a uint32
uint32_t ip_address::get_as_be32() const {
	return *((const uint32_t*)octets);
}

// compares ip addresses
bool ip_address::operator==(const ip_address& input) const {
	return *((const uint32_t*) input.octets) == *((const uint32_t*) octets);
}

// compares ip addresses
bool ip_address::operator!=(const ip_address& input) const {
	return !(*this == input);
}

// ip address ordering
bool ip_address::operator<(const ip_address& other) const {
	return *((const uint32_t*) octets) < *((const uint32_t*) other.octets);
}

// gets the address
string ip_address::to_string() const {
	char buf[16];
	sprintf(buf, "%u.%u.%u.%u", octets[0], octets[1], octets[2], octets[3]);
	return string(buf);
}

// returns true if the address is 0.0.0.0
bool ip_address::is_nil() const {
	return *((const uint32_t*) octets) == 0;
}

// returns true if the address is x.x.x.255
bool ip_address::is_broadcast() const {
	return octets[3] == 255;
}

// static function. validates an ip address that is given in numeric format.
bool ip_address::validate_numeric_ip(const string& input, ip_address* dest) {

	char buf[16];
	int values[4];
	int size = input.size();

	// quick sanity checks
	if (size == 0 || size > 16) {
		goto fail;
	}

	// check fields to make sure they are numeric
	if (sscanf(input.c_str(), "%d.%d.%d.%d", &values[0], &values[1], &values[2], &values[3]) != 4 ||
		values[0] < 0 || values[0] > 255 ||
		values[1] < 0 || values[1] > 255 ||
		values[2] < 0 || values[2] > 255 ||
		values[3] < 0 || values[3] > 255) {

		goto fail;
	}

	// final check to make sure there were no trailing contents after the given ip address
	sprintf(buf, "%d.%d.%d.%d", values[0], values[1], values[2], values[3]);
	if (strcmp(buf, input.c_str()) != 0) {
		goto fail;
	}

	// store result as needed
	if (dest != nullptr) {
		dest->set((uint8_t) values[0], (uint8_t) values[1], (uint8_t) values[2], (uint8_t) values[3]);
	}

	return true;

fail:

	if (dest != nullptr) {
		dest->clear();
	}
	return false;
}

// validates any general IP
bool ip_address::validate_ip(const string& input, ip_address* dest) {

	if (validate_numeric_ip(input, dest)) {
		return true;
	} else {
		ip_address temp;
		if (temp.resolve(input)) {
			if (dest != nullptr) {
				*dest = temp;
			}
			return true;
		} else {
			if (dest != nullptr) {
				dest->clear();
			}
			return false;
		}
	}
}
