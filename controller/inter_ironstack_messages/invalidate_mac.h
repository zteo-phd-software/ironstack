#pragma once
#include <list>
#include "../../common/mac_address.h"
#include "inter_ironstack_message.h"
using namespace std;

namespace ironstack {
class invalidate_mac : public inter_ironstack_message {
public:

	// constructor and destructor
	invalidate_mac();
	~invalidate_mac();

	// clears the message
	virtual void     clear();
	virtual string   to_string() const;

	// serialize and deserialize functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool     deserialize(const autobuf& buf);

	// publicly accessible fields
	list<mac_address> addresses_to_purge;
};
};
