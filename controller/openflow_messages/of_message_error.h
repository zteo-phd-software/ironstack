#pragma once

#include <arpa/inet.h>
#include <string>
#include "of_message.h"

// openflow error message
// author: Z. Teo (zteo@cs.cornell.edu)

using namespace std;
class of_message_error : public of_message {
public:

	of_message_error();
	virtual ~of_message_error() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	string base_error;
	string extended_error;
	autobuf error_data;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
