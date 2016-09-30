#include "mac_address.h"

// default constructor
mac_address::mac_address() {
	clear();
}

// clears the mac
void mac_address::clear() {
	memset(addr, 0, sizeof(addr));
}

// sets the mac to the 'not configured' address
void mac_address::set_nil() {
	memset(addr, 0, sizeof(addr));
}

// sets the mac to the broadcast address
void mac_address::set_broadcast() {
	memset(addr, 0xff, sizeof(addr));
}

// checks if it is a 'not configured' address
bool mac_address::is_nil() const {
	uint8_t null_addr[6] = {0,0,0,0,0,0};
	return memcmp(null_addr, addr, sizeof(addr)) == 0;
}

// checks if the address is a broadcast address
bool mac_address::is_broadcast() const {
	uint8_t bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	return memcmp(bcast_addr, addr, sizeof(addr)) == 0;
}

// setter method
mac_address& mac_address::operator=(const mac_address& other) {
	memcpy(addr, other.addr, sizeof(addr));
	return *this;
}

// setter method
mac_address& mac_address::operator=(const std::string& addr) {
	set(addr);
	return *this;
}

// equal comparison operator
bool mac_address::operator==(const mac_address& other) const {
	if (memcmp(addr, other.addr, sizeof(addr)) == 0) {
		return true;
	}

	return false;
}

// not equal comparison operator
bool mac_address::operator!=(const mac_address& other) const {
	return !((*this) == other);
}

// setter method
void mac_address::set_from_network_buffer(const void* addr_in) {
	memcpy(addr, addr_in, 6);
}

// setter method
void mac_address::set(const std::string& addr_in) {
	uint8_t counter;
	uint8_t current_octet = 0;
	uint8_t current_char;

	if (addr_in.size() != 17) {
		set_nil();
	} else {
		for (counter = 0; counter < addr_in.size(); counter++) {
			current_char = addr_in[counter];
			if ((counter+1) % 3 == 0) {
				if (current_char != ':') {
					goto fail;
				}

				addr[(counter+1)/3-1] = current_octet;
				current_octet = 0;
			} else {
				current_octet <<=4;
				if (current_char >= '0' && current_char <= '9') {
					current_octet |= (current_char - '0');
				} else if (current_char >= 'A' && current_char <= 'F') {
					current_octet |= (current_char-'A'+10);
				} else if (current_char >= 'a' && current_char <= 'f') {
					current_octet |= (current_char-'a'+10);
				} else {
					goto fail;
				}
			}
		}
		addr[(counter+1)/3-1] = current_octet;
	}
	return;

fail:
	set_nil();
}

// getter method
void mac_address::get(uint8_t* dest) const {
	memcpy(dest, addr, 6);
}

// generates a readable version of the mac address
std::string mac_address::to_string() const {
	std::string result;
	char buf[8];

	for (int counter = 0; counter < 6; counter++) {
		sprintf(buf, "%02x:", addr[counter]);
		result += buf;
	}

	result.resize(result.size()-1);
	return result;
}
