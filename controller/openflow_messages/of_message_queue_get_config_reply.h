#pragma once

#include <vector>
#include "of_message.h"
#include "../openflow_types/of_queue_config.h"

using namespace std;
class of_message_queue_get_config_reply : public of_message {
public:

	of_message_queue_get_config_reply();
	virtual ~of_message_queue_get_config_reply() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	uint16_t                port;
	vector<of_queue_config> queues;

	// serialization
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

private:
	// disable copy constructor and assignment operator
	of_message_queue_get_config_reply(const of_message_queue_get_config_reply& original);
	of_message_queue_get_config_reply& operator=(const of_message_queue_get_config_reply& original);

};
