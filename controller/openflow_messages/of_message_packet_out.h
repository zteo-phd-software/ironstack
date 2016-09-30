#pragma once

#include <memory>
#include <vector>
#include "../utils/of_common_utils.h"
#include "of_message.h"
#include "../ironstack_types/openflow_action_list.h"

class of_message_packet_out : public of_message {
public:

	of_message_packet_out();
	virtual ~of_message_packet_out() {};

	virtual void   clear();
	virtual string to_string() const;

	// user accessible fields
	uint32_t             buffer_id;
	bool                 use_port;
	uint16_t             in_port;	

	openflow_action_list action_list;
	autobuf              packet_data;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool     deserialize(const autobuf& input);

};
