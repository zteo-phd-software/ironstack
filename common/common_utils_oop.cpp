#include "common_utils_oop.h"

int cu_write_autobuf_to_socket(int socket_id, const autobuf& data)
{
	return cu_write_data_item(socket_id, data.get_content_ptr(), data.size());
}

int cu_read_autobuf_from_socket(int socket_id, autobuf& data)
{
	char* buf = NULL;
	int buf_size = 0;
	data.clear();

	buf_size =  cu_read_data_item_auto_alloc(socket_id, &buf);

	if (buf_size < 0 || buf == NULL)
		return -1;

	data.set_content(buf, buf_size);
	free(buf);

	return buf_size;
}


template <class T> bool cu_write_list_to_file(const std::string& filename, const std::list<T>& item_list)
{
	typename std::list<T>::const_iterator iterator;
	FILE* fp = fopen(filename.c_str(), "wb");
	uint32_t size = 0;
	autobuf current;

	if (fp == NULL)
		return false;

	size = htonl(item_list.size());
	if (!fwrite(&size, sizeof(size), 1, fp) != 1)
		goto fail;

	for (iterator = item_list.begin(); iterator != item_list.end(); iterator++)
	{
		current = iterator->serialize();
		size = htonl(current.size());
		if (!fwrite(&size, sizeof(size), 1, fp) != 1)
			goto fail;

		if (fwrite(current.get_content_ptr(), current.size(), 1, fp) != 1)
			goto fail;
	}

	fclose(fp);
	return true;

fail:
	fclose(fp);
	return false;
}

template <class T> bool cu_read_list_from_file(const std::string& filename, std::list<T>& item_list)
{
	T current_item;
	uint32_t number_of_items;
	uint32_t counter;
	uint32_t chunk_size;
	char* buf = NULL;

	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		return false;

	if (fread(&number_of_items, sizeof(number_of_items), 1, fp) != 1)
		goto fail;
	number_of_items = ntohl(number_of_items);

	for (counter = 0; counter < number_of_items; counter++)
	{
		if (fread(&chunk_size, sizeof(chunk_size), 1, fp) != 1)
			goto fail;
		chunk_size = ntohl(chunk_size);

		buf = (char*)malloc(chunk_size);
		if (buf == NULL)
			goto fail;

		if (fread(buf, chunk_size, 1, fp) != 1
			|| !current_item.deserialize(autobuf(buf, chunk_size)))
		{
			free(buf);
			goto fail;
		}

		free(buf);
		item_list.push_back(current_item);
	}	

	fclose(fp);
	return true;

fail:
	item_list.clear();
	fclose(fp);
	return false;
}

uint32_t calculate_checksum32(const autobuf& buf, uint32_t size = 0)
{
	uint32_t result = 0;
	if (size == 0)
		size = buf.size();

	for (uint32_t counter = 0; counter < size; counter++)
		result += buf[counter];

	// calculate one's complement
	result += 2;
	result = ~result;

	return result;
}

uint32_t calculate_checksum32(const void* buf, uint32_t size)
{
	autobuf buf2;
	buf2.inherit_read_only(buf, size);
	return calculate_checksum32(buf2);
}

uint16_t calculate_checksum16(const autobuf& buf, uint32_t size = 0)
{
	uint16_t result = 0;
	if (size == 0)
		size = buf.size();

	for (uint32_t counter = 0; counter < size; counter++)
		result += buf[counter];

	result += 2;
	result = ~result;

	return result;
}

uint16_t calculate_checksum16(const void* buf, uint32_t size)
{
	autobuf buf2;
	buf2.inherit_read_only(buf, size);
	return calculate_checksum16(buf2);
}

std::string get_input(const std::string& message)
{
	std::string result;
	printf("%s", message.c_str());
	std::getline(std::cin, result);
	return result;
}

std::vector<std::string> string_utils::tokenize(const std::string& original, const std::string& separators) {

	std::vector<std::string> result;

	// no separators, no action
	if (separators.size() == 0) {
		result.push_back(original);
		return result;
	}
	
	bool found;
	std::string current_string;
	for (char current : original) {

		found = false;
		for (char separator : separators) {
			if (current == separator) {
				found = true;
				break;
			}
		}

		if (!found) {
			current_string.push_back(current);
		} else if (current_string.size() > 0) {
			result.push_back(std::move(current_string));
			current_string.clear();
		}
	}

	if (current_string.size() > 0) {
		result.push_back(std::move(current_string));
	}

	return result;
}

std::vector<std::string> string_utils::tokenize(const std::string& original, char separator) {
	std::string separators;
	separators.push_back(separator);
	return tokenize(original, separators);
}

std::vector<std::pair<std::string, std::string> > string_utils::generate_key_value_pair(const std::string& original, const std::string& separators) {

	std::vector<std::pair<std::string, std::string> > result;

	// first tokenize by separators
	std::vector<std::string> tokens = tokenize(original, separators);

	// tokenize each key/value pair
	std::pair<std::string, std::string> one_pair;
	for (const auto& line : tokens) {
		std::vector<std::string> key_value_pair = tokenize(line, " =");

		if (key_value_pair.size() != 2) {
			// ignore malformed specs and continue
			continue;
		} else {
			one_pair.first = key_value_pair[0];
			one_pair.second = key_value_pair[1];
			result.push_back(std::move(one_pair));
		}
	}

	return result;
}

std::vector<std::pair<std::string, std::string>> string_utils::generate_key_value_pair(const std::string& input) {

	std::vector<std::pair<std::string, std::string>> result;
	std::string key;
	std::string value;

	uint32_t len = input.size();
	uint32_t offset = 0;

	while (offset < len) {

		key.clear();
		value.clear();

		// first trim preceding whitespace
		while (is_whitespace(input[offset])) {
			if (++offset == len) goto done;
		}

		// grab until non alphanumeric char
		while(is_alphanumeric(input[offset]) || input[offset] == '_' || input[offset] == '.' || input[offset] == ':' || input[offset] == '-') {
			key.push_back(input[offset]);
			if (++offset == len) goto done;
		}

		// ignore whitespace following key (trying to advance to the '=')
		while(is_whitespace(input[offset])) {
			if (++offset == len) goto done;
		}

		if (input[offset++] != '=') {
			goto done;
		}

		// ignore whitespace following =
		while(is_whitespace(input[offset])) {
			if (++offset == len) goto done;
		}

		// grab the value
		while (is_alphanumeric(input[offset]) || input[offset] == '_' || input[offset] == '.' || input[offset] == ':' || input[offset] == '-') {
			value.push_back(input[offset]);
			if (++offset == len) break;
		}

		result.push_back(std::make_pair(key, value));
	}

done:

	return result;
}

std::string string_utils::trim_leading_and_trailing_whitespace(const std::string& input) {
	std::string result;	
	
	if (input.empty()) return result;

	uint32_t len = input.size();
	uint32_t offset_start=0;
	uint32_t offset_end = input.size()-1;
	for (int counter = 0; counter < (int)len; ++counter) {
		if (input[counter] == ' ') {
			offset_start = counter+1;
		} else {
			break;
		}
	}

	for (int counter = len-1; counter >= 0; --counter) {
		if (input[counter] == ' ') {
			offset_end = counter;
		} else {
			break;
		}
	}

	if (offset_start < offset_end) {
		result = input.substr(offset_start, offset_end-offset_start);
	}

	return result;
}

bool string_utils::is_whitespace(char input) {
	if (input == ' ' || input == '\t' || input == '\n' || input == '\r') {
		return true;
	}
	return false;
}

bool string_utils::is_alphanumeric(char input) {
	if ((input >= '0' && input <= '9') || (input >= 'a' && input <= 'z') || (input >= 'A' && input <= 'Z')) {
		return true;
	}
	return false;
}

bool string_utils::string_to_uint32(const std::string& input, uint32_t* storage) {
	return sscanf(input.c_str(), "%u", storage) == 1;
}

bool string_utils::string_to_uint16(const std::string& input, uint16_t* storage) {
	return sscanf(input.c_str(), "%hu", storage) == 1;
}

bool string_utils::string_to_int(const std::string& input, int* storage) {
	return sscanf(input.c_str(), "%d", storage) == 1;
}

uint32_t string_utils::string_to_constrained_buf(const std::string& input, void* buf, uint32_t buf_size) {
	uint32_t result = 0;
	for (uint32_t counter = 0; counter < buf_size; ++counter) {
		uint8_t current_char = input.c_str()[counter];
		if (input.c_str()[counter] != '\0') {
			((uint8_t*)buf)[counter] = current_char;
			++result;
		} else {
			memset(((uint8_t*)buf)+counter, 0, buf_size-counter);
			break;
		}
	}
	return result;
}

std::string string_utils::constrained_buf_to_string(const void* buf, uint32_t buf_size) {
	std::string result;
	for (uint32_t counter = 0; counter < buf_size; ++counter) {
		uint8_t current_char = ((uint8_t*)buf)[counter];
		if (current_char != '\0') {
			result.push_back(current_char);
		} else {
			break;
		}
	}
	return result;
}


