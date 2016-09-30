#pragma once

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <vector>
#include "of_message.h"
#include "../ironstack_types/openflow_flow_description.h"

using namespace std;
class of_message_modify_flow : public of_message {
public:

	of_message_modify_flow();
	virtual ~of_message_modify_flow() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	openflow_flow_description flow_description;

	enum ofp_flow_mod_command	command;
	uint16_t				idle_timeout;
	uint16_t				hard_timeout;
	uint32_t				buffer_id;

	bool						use_out_port;
	uint16_t				out_port;
	bool						flag_send_flow_removal_message;
	bool						flag_check_overlap;
	bool						flag_emergency_flow;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
