#pragma once

#include <string>
#include "of_message.h"
#include "../ironstack_types/openflow_switch_config.h"

using namespace std;
class of_message_get_config_reply : public of_message {
public:

	of_message_get_config_reply();
	virtual ~of_message_get_config_reply() {};

	virtual void clear();
	virtual string to_string() const;

	openflow_switch_config config;

	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
