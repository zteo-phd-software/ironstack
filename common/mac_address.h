#ifndef __MEDIA_ACCESS_CONTROL_ADDRESS
#define __MEDIA_ACCESS_CONTROL_ADDRESS

#include <string>
#include <stdio.h>
#include <memory.h>

class mac_address {
public:

	mac_address();
	mac_address(uint8_t* raw_addr_in) { set_from_network_buffer(raw_addr_in); }
	mac_address(const std::string& addr_in) { set(addr_in); }

	bool operator<(const mac_address& other) const { 

		for (uint32_t counter = 0; counter < 6; ++counter) {
			if (addr[counter] < other.addr[counter]) {
				return true;
			} else if (addr[counter] > other.addr[counter]) {
				return false;
			}
		}

		return false;
		
	};

//	return ((*((const uint32_t*) &addr[0]) << 16) | (*(const uint16_t*) &addr[4])) < ((*(const uint32_t*) &other.addr[0]) | (*(const uint16_t*) &other.addr[4])); }
	mac_address& operator=(const mac_address& other);	
	mac_address& operator=(const std::string& addr_in);

	// convenience functions
	void clear();
	void set_nil();
	void set_broadcast();

	bool is_nil() const;
	bool is_broadcast() const;

	// comparison operators
	bool operator==(const mac_address& other) const;
	bool operator!=(const mac_address& other) const;

	// getter and setter methods
	void set_from_network_buffer(const void* raw_addr_in);
	void set(const std::string& addr_in);
	void get(uint8_t* dest) const;

	// tostring method -- returns just the address in a string
	// no extra debug info
	std::string to_string() const;

private:

	uint8_t addr[6];

};

namespace std {
	template <>
	struct hash<mac_address> : public unary_function<mac_address, size_t> {
		size_t operator()(const mac_address& value) const {
			uint8_t buf[6];
			value.get(buf);
			return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]) + buf[4] + buf[5];
		}
	};
};


#endif
