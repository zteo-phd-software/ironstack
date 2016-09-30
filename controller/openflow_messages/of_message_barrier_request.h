#pragma once

#include <string>
#include <arpa/inet.h>
#include "of_message.h"

// barrier request message
// author: Z. Teo (zteo@cs.cornell.edu)

using namespace std;
class of_message_barrier_request : public of_message {
public:

	// constructors
	of_message_barrier_request();
	virtual ~of_message_barrier_request() {};

	// universal message functions
	virtual void clear();
	virtual string to_string() const;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};
