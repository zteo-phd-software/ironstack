#pragma once

#include <string>
#include "of_message.h"
#include "../ironstack_types/of_types.h"
#include "../../common/mac_address.h"
#include "../../common/std_packet.h"
#include "../utils/of_common_utils.h"

using namespace std;
class raw_packet;

// defines the class for an incoming message
class of_message_packet_in : public of_message {
public:

	of_message_packet_in();
	virtual ~of_message_packet_in() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible data fields
	uint32_t  buffer_id;
	uint16_t  in_port;
	uint16_t  actual_message_len;	// len of the message in the switch
																// a shorter message may be sent through
																// packet_in depending on miss_len as
																// set by SET_CONFIG (default 128)
																// on init, hal sets this to 65535 (max)

	bool      summarized;						// flag to indicate if the packet_in data
																// did not contain the entire original packet
	bool      reason_no_match;
	bool      reason_action;
	autobuf   pkt_data;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

	// use this function to display/suppress payload display
	static void show_contents(bool state);

private:

	// private function to append readable ethernet frame breakdowns
	string get_breakdowns() const;
	static bool display_payload;
};

// factory class to generate the correct packet type
class packet_factory {
public:

	// inspects headers and calls the appropriate packet generator
	// in the of_packets class
	static raw_packet* instantiate(autobuf* buf, const of_message_packet_in& msg);
};
