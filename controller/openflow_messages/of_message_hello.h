#pragma once

// file: of_message_hello.h/cpp
// author: Z. Teo (zteo@cs.cornell.edu)
//
// classes: of_message_hello

#include "of_message.h"

using namespace std;
class of_message_hello : public of_message {
public:
	
	of_message_hello();
	virtual ~of_message_hello() {};

	virtual void clear();
	virtual string to_string() const;

	// inherited from serializable
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};
