#ifndef __OPENFLOW_SWITCH_CONFIGURATION
#define __OPENFLOW_SWITCH_CONFIGURATION

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include "../../common/autobuf.h"
#include "../../common/openflow.h"

class openflow_switch_config {
public:

	void clear();
	std::string to_string() const;

	bool frag_normal;		// no special handling for fragments
	bool frag_drop;			// drop fragments
	bool frag_reasm;		// reassemble IP fragments (only if switch capabilities OFPC_IP_REASM is set)
	bool frag_mask;			// ?? not described

	uint16_t max_msg_send_len;	// maximum length of message summary to be sent on packet-in events

	autobuf serialize() const;
	bool deserialize(const autobuf& input);

};

#endif
