#ifndef __OPENFLOW_VENDOR_MESSAGE
#define __OPENFLOW_VENDOR_MESSAGE

#include <arpa/inet.h>
#include <memory.h>
#include <string>
#include "of_message.h"

class of_message_vendor : public of_message {
public:

	of_message_vendor();
	virtual ~of_message_vendor() {};

	virtual void clear();
	std::string to_string() const;

	// user-accessible fields
	uint32_t vendor;
	autobuf request;

	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};

#endif
