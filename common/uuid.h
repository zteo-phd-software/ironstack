#ifndef __UUID
#define __UUID

// standard C99 include files
#include <string>
#include <stdint.h>
#include <memory.h>

// internal include files
#include "autobuf.h"
#include "serializable.h"
#include "common_utils.h"

// invalid uuid  : 00000000-0000-0000-0000-0000000000000000
// broadcast uuid: ffffffff-ffff-ffff-ffff-ffffffffffffffff
// upon construction, default uuid is invalid.

class uuid : public serializable
{
public:

	// constructors
	uuid();
	uuid(const std::string& uuid_string);
	uuid(const uuid& other);
	uuid& operator=(const uuid& other);
	~uuid();

	// clear
	void clear();
	
	// getter methods
	std::string get_uuid() const;
	std::string get_description() const;

	// check methods
	bool is_uuid_broadcast() const;
	bool is_nil() const;

	// serialization/deserialization tools
	virtual autobuf serialize() const;
	virtual bool deserialize(const autobuf& input);

	// comparison operators
	bool operator==(const uuid& other) const;
	bool operator!=(const uuid& other) const;
	bool operator< (const uuid& other) const;

	// generate a random uuid
	static uuid generate();

	// generates the broadcast and nil uuids respectively
	static uuid broadcast_uuid();
	static uuid nil_uuid();

private:

	std::string description;
	uint8_t uuid_buf[16];
};

#endif
