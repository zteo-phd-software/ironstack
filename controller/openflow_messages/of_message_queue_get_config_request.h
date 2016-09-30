#pragma once

#include <arpa/inet.h>
#include <memory.h>
#include <string>
#include "of_message.h"

using namespace std;
class of_message_queue_get_config_request : public of_message{
public:

	of_message_queue_get_config_request();
	virtual ~of_message_queue_get_config_request() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	uint16_t port;

	// from the serialization class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};
