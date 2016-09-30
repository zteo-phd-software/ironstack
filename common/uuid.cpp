#include "uuid.h"

// default constructor, creates invalid uuid
uuid::uuid()
{
	description = "";
	memset(uuid_buf, 0, sizeof(uuid_buf));
}

// shortcut constructor, creates uuid from string
uuid::uuid(const std::string& uuid_string)
{
	uint32_t counter;
	uint32_t write_index = 0;
	uint8_t current_byte_value;
	uint8_t next_write_value;
	bool write_low_half;

	if (uuid_string.size() != 36)
	{
		description = "";
		memset(uuid_buf, 0, sizeof(uuid_buf));
		return;
	}

	write_low_half = true;
	next_write_value = 0;
	for (counter = 0; counter < 36; counter++)
	{
		if (counter == 8 || counter == 13 || counter == 18 || counter == 23)
			continue;
		
		current_byte_value = uuid_string[counter];
		if (current_byte_value >= 48 && current_byte_value <= 57)
			current_byte_value -= 48;
		else if (current_byte_value >= 65 && current_byte_value <=70)
			current_byte_value -= 55;
		else if (current_byte_value >= 97 && current_byte_value <= 102)
			current_byte_value -= 87;
		else
		{
			description = "";
			memset(uuid_buf, 0, sizeof(uuid_buf));
			return;
		}

		if (write_low_half)
		{
			write_low_half = false;
			next_write_value = current_byte_value;
		}
		else
		{
			current_byte_value <<= 4;
			next_write_value |= current_byte_value;
			uuid_buf[write_index++] = next_write_value;
			write_low_half = true;
		}
	}
}

// copy constructor
uuid::uuid(const uuid& other)
{
	description = other.description;
	memcpy(uuid_buf, other.uuid_buf, sizeof(uuid_buf));
}

// assignment operator
uuid& uuid::operator=(const uuid& other)
{
	if (this == &other)
		return *this;
	description = other.description;
	memcpy(uuid_buf, other.uuid_buf, sizeof(uuid_buf));
	return *this;
}

// destructor
uuid::~uuid()
{

}

// clear command
void uuid::clear()
{
	description = "";
	memset(uuid_buf, 0, sizeof(uuid_buf));
}
// gets uuid as a std string
std::string uuid::get_uuid() const
{
	char string_buffer[37];
	int write_index = 0;
	char low_byte, high_byte;
	uint32_t counter;

	memset(string_buffer, 0, sizeof(string_buffer));
	for (counter = 0; counter < sizeof(uuid_buf); counter++)
	{
		if (write_index == 8 || write_index == 13 || write_index == 18 || write_index == 23)
			string_buffer[write_index++] = '-';

		low_byte  = uuid_buf[counter] & 0x0F;
		high_byte = (uuid_buf[counter] & 0xF0) >> 4;

		if (low_byte < 10)
			string_buffer[write_index] = '0'+low_byte;
		else
			string_buffer[write_index] = 'a'+low_byte-10;
		write_index++;
		
		if (high_byte < 10)
			string_buffer[write_index] = '0'+high_byte;
		else
			string_buffer[write_index] = 'a'+high_byte-10;
		write_index++;
	}
	string_buffer[write_index] = '\0';

	return std::string(string_buffer);
}

// gets the description
std::string uuid::get_description() const
{
	return description;
}

// check if uuid is broadcast uuid
bool uuid::is_uuid_broadcast() const
{
	uint8_t bcast_uuid[16];
	memset(bcast_uuid, 255, sizeof(bcast_uuid));

	if (memcmp(uuid_buf, bcast_uuid, sizeof(uuid_buf)) != 0)
		return false;
	else
		return true;
}
// check if uuid is invalid uuid
bool uuid::is_nil() const
{
	uint8_t nil_uuid[16];
	memset(nil_uuid, 0, sizeof(nil_uuid));

	if (memcmp(uuid_buf, nil_uuid, sizeof(uuid_buf)) != 0)
		return false;
	else
		return true;
}

// serialization
autobuf uuid::serialize() const
{
	serializer writer;

	writer.append_length_and_payload(description.c_str(), description.size()+1);
	writer.append_length_and_payload(uuid_buf, sizeof(uuid_buf));

	return writer.get_serialization();
}

// deserializes a uuid
bool uuid::deserialize(const autobuf& input)
{
	deserializer reader(input);
	autobuf uuid_autobuf;
	autobuf description_autobuf;

	clear();

	if (!reader.get_next_length_and_autobuf(description_autobuf)
		|| !reader.get_next_length_and_autobuf(uuid_autobuf)
		|| (description_autobuf.size() > 0 && description_autobuf[description_autobuf.size()-1] != '\0')
		|| uuid_autobuf.size() != sizeof(uuid_buf))
		return false;
	
	description = (const char*) description_autobuf.get_content_ptr();
	memcpy(uuid_buf, uuid_autobuf.get_content_ptr(), sizeof(uuid_buf));
	return true;
}

// comparison operators
bool uuid::operator==(const uuid& other) const
{
	if (memcmp(uuid_buf, other.uuid_buf, sizeof(uuid_buf)) != 0 || description != other.description)
		return false;
	return true;
}

bool uuid::operator!=(const uuid& other) const
{
	return !(other == *this);
}

bool uuid::operator<(const uuid& other) const
{
	uint32_t counter;
	for (counter = 0; counter < sizeof(uuid_buf); counter++)
	{
		if (uuid_buf[counter] < other.uuid_buf[counter])
			return true;
		else if (uuid_buf[counter] > other.uuid_buf[counter])
			return false;
	}
	return false;
}

// generates a random uuid
uuid uuid::generate()
{
	uuid random_uuid;
	uint32_t counter;

	cu_reseed_random_number_generator();
	for (counter = 0; counter < sizeof(random_uuid.uuid_buf); counter++)
		random_uuid.uuid_buf[counter] = (uint8_t) (cu_generate_random_number(0, 15) | (cu_generate_random_number(0, 15) << 4));
	random_uuid.description = "";

	return random_uuid;
}

// generates the broadcast uuid
uuid uuid::broadcast_uuid()
{
	uuid result;
	result.description = "";
	memset(result.uuid_buf, 0xff, sizeof(result.uuid_buf));

	return result;
}

// generates the invalid uuid
uuid uuid::nil_uuid()
{
	return uuid();
}


