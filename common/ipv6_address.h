#ifndef __IPV6_ADDRESS_TYPE
#define __IPV6_ADDRESS_TYPE

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <arpa/inet.h>
#include "common_utils.h"

class ipv6_address {
public:

	// constructors
	ipv6_address();
	ipv6_address(const ipv6_address& input);
	ipv6_address(const std::string& input);
  ipv6_address(uint16_t half_word1,
                uint16_t half_word2,
                uint16_t half_word3,
                uint16_t half_word4,
                uint16_t half_word5,
                uint16_t half_word6,
                uint16_t half_word7,
                uint16_t half_word8);

	// setter and getter methods
	virtual void clear();
	bool resolve(const std::string& input);
	bool set(const ipv6_address& input);
	bool set(const std::string& input);
  bool set(uint16_t half_word1,
                uint16_t half_word2,
                uint16_t half_word3,
                uint16_t half_word4,
                uint16_t half_word5,
                uint16_t half_word6,
                uint16_t half_word7,
                uint16_t half_word8);
	void set_from_network_buffer(const void* buf);

	ipv6_address& operator=(const ipv6_address& input);
	ipv6_address& operator=(const std::string& input);

	uint16_t operator[](int index) { if (index < 0 || index > 7) { abort(); } else { return half_words[index];} }
	void get(void* buf) const { memcpy(buf, half_words, 8); }

	// comparators
	bool operator==(const ipv6_address& input) const;
	bool operator!=(const ipv6_address& input) const;

	virtual std::string to_string() const;
	void get_address_network_order(void* buf) const;

	// validation methods
	bool is_nil() const;
	bool is_multicast() const;

	// useful public tools
	static bool validate(const std::string& input);

	// converts the string form of the address into the internal form
	bool extract_address(const std::string& input);

protected:

	uint16_t half_words[8];

};

namespace std {
	template <>
	struct hash<ipv6_address> : public unary_function<ipv6_address, size_t> {
		size_t operator()(const ipv6_address& value) const {
			uint8_t buf[4];
			value.get(buf);
			return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
		}
	};
};


#endif
