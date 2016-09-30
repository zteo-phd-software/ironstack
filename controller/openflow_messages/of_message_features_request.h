#pragma once

#include "of_message.h"
#include "../../common/mac_address.h"

// requests the switch to send its available features
// author: Z. Teo (zteo@cs.cornell.edu)

using namespace std;
class of_message_features_request : public of_message {
public:
	
	of_message_features_request();
	virtual ~of_message_features_request() {};

	virtual void clear();
	virtual string to_string() const;

	// inherited from serializable
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};
