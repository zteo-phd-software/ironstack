#pragma once
#include "inter_ironstack_message.h"
using namespace std;

namespace ironstack {
class ethernet_pong : public inter_ironstack_message {
public:

	// constructor and destructor
	ethernet_pong();
	~ethernet_pong();

	// generates a readable form of the message
	virtual string to_string() const;
	
	// serialize and deserialize functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool     deserialize(const autobuf& input);

};
};
