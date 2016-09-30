#pragma once

#include <string>
#include "of_message.h"

class of_message_get_config_request : public of_message {
public:

	of_message_get_config_request();
	virtual ~of_message_get_config_request() {};

	virtual void clear();
	virtual string to_string() const;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};
