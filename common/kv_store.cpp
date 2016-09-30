#include "kv_store.h"

// constructor
kv_store::kv_store()
{
}

// shortcut constructor
kv_store::kv_store(const autobuf& serialized_input)
{
	deserialize(serialized_input);
}

// clears the object
void kv_store::clear()
{
	store.clear();
}

// dumps the object into a serializable form
autobuf kv_store::serialize() const
{
	autobuf result;
	uint32_t size_field = store.size();

	// pack the number of fields first
	size_field = htonl(size_field);
	result.append(&size_field, sizeof(size_field));

	// pack individual kv pairs
	for (std::list<std::pair<std::string, autobuf> >::const_iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		// size of key (includes null terminating char)
		size_field = htonl(iterator->first.size()+1);
		result.append(&size_field, sizeof(size_field));
		result.append(iterator->first.c_str(), iterator->first.size()+1);

		// insert value
		size_field = htonl(iterator->second.size());
		result.append(&size_field, sizeof(size_field));
		result += iterator->second;
	}

	return result;
}

// deserializes the object into a readable form
bool kv_store::deserialize(const autobuf& serialized_input)
{
	clear();

	std::string current_key;
	autobuf key_automem;
	autobuf current_value;
	std::pair<std::string, autobuf> current_pair;
	uint32_t store_size = 0;
	uint32_t current_offset = 0;
	uint32_t size_field = 0;
	uint32_t input_size = serialized_input.size();

	// sanity check
	if (input_size < sizeof(size_field))
		goto fail;
	
	serialized_input.read_range(&size_field, current_offset, current_offset+sizeof(size_field)-1);
	current_offset += sizeof(size_field);
	store_size = ntohl(size_field);

	for (uint32_t counter = 0; counter < store_size; counter++)
	{
		// read in key size
		if (current_offset+sizeof(size_field) > input_size)
			goto fail;
		serialized_input.read_range(&size_field, current_offset, current_offset+sizeof(size_field)-1);
		current_offset += sizeof(size_field);
		size_field = ntohl(size_field);

		// read in key
		if (current_offset+size_field > input_size)
			goto fail;
		autobuf key_auto_mem;
		serialized_input.read_range(key_auto_mem, current_offset, current_offset+size_field-1);
		current_offset += size_field;
		if (key_automem[size_field-1] != '\0')
			goto fail;
		current_key = (char*) key_automem.get_content_ptr();

		// read in value size
		if (current_offset+sizeof(size_field) > input_size)
			goto fail;
		serialized_input.read_range(&size_field, current_offset, current_offset+sizeof(size_field)-1);
		current_offset += sizeof(size_field);
		size_field = ntohl(size_field);

		// read in value
		if (current_offset+size_field > input_size)
			goto fail;
		serialized_input.read_range(current_value, current_offset, current_offset+size_field-1);
		current_offset += size_field;

		// add to store
		current_pair.first = current_key;
		current_pair.second = current_value;
		store.push_back(current_pair);
	}

	return true;

fail:

	clear();
	return false;
}

// read a series of values from a kv store
bool kv_store::read(const std::string& key, const std::string& format, ...) const
{
	bool found = false;
	va_list args;

	// scan through the kv store for the serialized object
	for (std::list<std::pair<std::string, autobuf> >::const_iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		if (iterator->first.compare(key) == 0)
		{
			// make sure it is a string that can be sscanf'd on
			if (iterator->second.size() == 0 ||
				((const char*)(iterator->second.get_content_ptr()))[iterator->second.size()-1] != '\0')
				return false;

			va_start(args, format);
			vsscanf((const char*)iterator->second.get_content_ptr(), format.c_str(), args);
			va_end(args);
			found = true;
			break;
		}
	}

	if (!found)
		return false;

	return true;
}

// reads a string from the serialized object
bool kv_store::read_string(const std::string& key, std::string& result) const
{
	// scan through the kv store for the string
	for (std::list<std::pair<std::string, autobuf> >::const_iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		if (iterator->first.compare(key) == 0)
		{
			// make sure it is a string
			if (iterator->second.size() == 0 ||
				((const char*) (iterator->second.get_content_ptr()))[iterator->second.size()-1] != '\0')
				return false;

			result = (const char*) iterator->second.get_content_ptr();
			return true;
		}
	}

	result.clear();
	return false;
}

// reads an autobuf from the serialized object (useful for binary objects)
bool kv_store::read_raw(const std::string& key, autobuf& output) const
{
	// scan through the kv store for the string
	for (std::list<std::pair<std::string, autobuf> >::const_iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		if (iterator->first.compare(key) == 0)
		{
			output = iterator->second;
			return true;
		}
	}

	output.clear();
	return false;
}

// checks if a key exists
bool kv_store::exist(const std::string& key) const
{
	for (std::list<std::pair<std::string, autobuf> >::const_iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		if (iterator->first.compare(key) == 0)
			return true;
	}

	return false;
}

// retrieves a list of all the keys
std::list<std::string> kv_store::get_all_keys() const
{
	std::list<std::string> result;
	for (std::list<std::pair<std::string, autobuf> >::const_iterator iterator = store.begin(); iterator != store.end(); iterator++)
		result.push_back(iterator->first);

	return result;
}

// writes a value to the serialized object
bool kv_store::write(const std::string& key, const std::string& format, ...)
{
	int bytes_required = 0;
	va_list args;
	std::pair<std::string, autobuf> new_pair;

	// dump string into a buffer
	va_start(args, format);
	bytes_required = vsnprintf(NULL, 0, format.c_str(), args)+1;
	va_end(args);
	char* buf = (char*) malloc(bytes_required);
	if (buf == NULL)
		return false;
	va_start(args, format);
	vsnprintf(buf, bytes_required, format.c_str(), args);
	va_end(args);

	// search for existing key-value pair
	for (std::list<std::pair<std::string, autobuf> >::iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		if (iterator->first.compare(key) == 0)
		{
			// update value associated with this key
			iterator->second.set_content(buf, bytes_required);
			free(buf);
			return true;
		}
	}

	// doesn't exist, add the kv-pair
	new_pair.first = key;
	new_pair.second = autobuf(buf, bytes_required);
	free(buf);
	store.push_back(new_pair);

	return true;
}

// writes a string for the kv-pair
bool kv_store::write_string(const std::string& key, const std::string& value)
{
	autobuf autobuf_string(value.c_str(), value.size()+1);
	return write_raw(key, autobuf_string);
}

// writes an autobuf to the serialized object (useful for binary objects)
bool kv_store::write_raw(const std::string& key, const autobuf& raw_value)
{
	std::pair<std::string, autobuf> current_pair;

	for (std::list<std::pair<std::string, autobuf> >::iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		if (iterator->first.compare(key) == 0)
		{
			iterator->second = raw_value;
			return true;
		}
	}

	current_pair.first = key;
	current_pair.second = raw_value;
	store.push_back(current_pair);
	return true;
}

// writes a binary block into the serialization object (useful if buffer is not an autobuf)
bool kv_store::write_raw(const std::string& key, const void* data, int len)
{
	autobuf auto_buf(data, len);
	return write_raw(key, auto_buf);
}

// erases a key-value pair from the serialization
bool kv_store::erase(const std::string& key)
{
	for (std::list<std::pair<std::string, autobuf> >::iterator iterator = store.begin(); iterator != store.end(); iterator++)
	{
		if (iterator->first.compare(key) == 0)
		{
			store.erase(iterator);
			return true;
		}
	}

	return false;
}

