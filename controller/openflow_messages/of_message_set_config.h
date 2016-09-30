#pragma once

#include <string>
#include "of_message.h"

using namespace std;
class of_message_set_config : public of_message {
public:

	of_message_set_config();
	virtual ~of_message_set_config() {};

	virtual void clear();
	virtual string to_string() const;

	// one of these four options have to be set
	bool frag_normal;			// no special handling for fragments
	bool frag_drop;				// drop fragments
	bool frag_reasm;			// reassemble fragments (only if switch capabilities
												// OFPC_IP_REASM is set)
	bool frag_mask;				// ?? not described

	uint16_t max_msg_send_len;	// maximum message size sent on packet-in events

	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
