#pragma once

#include <string>
#include <stdint.h>
#include "../../common/autobuf.h"
using namespace std;

// base class for inter-ironstack messages
namespace ironstack {
class inter_ironstack_message {
public:

	const uint8_t VERSION = 1;

	// define some message types here
	enum class message_t : uint16_t {
		UNKNOWN = 0,
		ETHERNET_PING = 1,
		ETHERNET_PONG = 2,
		INVALIDATE_MAC = 3
	};

	// constructor and destructor
	inter_ironstack_message();
	virtual ~inter_ironstack_message();

	// universal message functions
	virtual void     clear();
	virtual string   to_string() const;

	// get the message type in string form
	string           get_message_type_string() const;

	// serialize and deserialize functions
	virtual uint32_t serialize(autobuf& dest) const=0;
	virtual bool     deserialize(const autobuf& input)=0;

	// publicly accessible fields
	uint8_t   version;
	message_t msg_type;
	uint32_t  xid;

protected:

	// define the serialization structure here
	typedef struct {
		uint8_t version;
		uint8_t msg_type[2];
		uint8_t xid[4];
		uint8_t payload_size[2];
		uint8_t data[0];
	} ironstack_message_hdr;

};};
