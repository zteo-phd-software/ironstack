#pragma once

#include <string>
#include "of_message.h"

// openflow echo request message
// author: Z. Teo (zteo@cs.cornell.edu)

using namespace std;
class of_message_echo_request : public of_message {
public:

	// constructor
	of_message_echo_request();
	virtual ~of_message_echo_request() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible portion
	autobuf data;

	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

	static void show_contents(bool state);

private:
	static bool display_content;
};
