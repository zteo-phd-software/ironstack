#pragma once

#include <string>
#include "of_message.h"

// barrier class
// author: Z. Teo (zteo@cs.cornell.edu)

using namespace std;
class of_message_barrier_reply : public of_message {
public:

	// constructors
	of_message_barrier_reply();
	virtual ~of_message_barrier_reply() {};

	// universal message functions
	virtual void clear();
	virtual string to_string() const;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};
