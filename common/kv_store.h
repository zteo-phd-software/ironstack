#ifndef __KEY_VALUE_STORE
#define __KEY_VALUE_STORE

#include <list>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "autobuf.h"
#include "serializable.h"

// serialized object subclassed from autobuf
class kv_store : protected serializable
{
public:

	kv_store();
	kv_store(const autobuf& serialized_input);
	~kv_store() {};

	// reset function
	void clear();

	// required implementation for serializable class
	virtual autobuf serialize() const;

	// deserialization from autobuf
	virtual bool deserialize(const autobuf& serialized_input);

	// query functions
	bool read(const std::string& key, const std::string& format, ...) const;
	bool read_string(const std::string& key, std::string& result) const;
	bool read_raw(const std::string& key, autobuf& output) const;
	bool exist(const std::string& key) const;
	std::list<std::string> get_all_keys() const;
	uint32_t size() const;

	// write functions
	bool write(const std::string& key, const std::string& format, ...);
	bool write_string(const std::string& key, const std::string& value);
	bool write_raw(const std::string& key, const autobuf& raw_value);
	bool write_raw(const std::string& key, const void* data, int len);

	// erase functions
	bool erase(const std::string& key);

private:

	std::list<std::pair<std::string, autobuf> > store;
};

#endif
