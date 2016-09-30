#include "string_utils.h"

// tokenzies a string by a single character separator
vector<string> string_utils::tokenize(const string& original, char separator) {
	string new_separator;
	new_separator.push_back(separator);
	return tokenize(original, new_separator);
}

// tokenizes a string by a string separator
vector<string> string_utils::tokenize(const string& original, const char* separator) {
	string new_separator(separator);
	return tokenize(original, new_separator);
}

// tokenizes a string by a single string separator
vector<string> string_utils::tokenize(const string& original, const string& separator) {
	set<string> separators;
	separators.insert(separator);
	return tokenize(original, separators);
}

// tokenizes a string by a set of separators
vector<string> string_utils::tokenize(const string& original, const set<string>& separators) {
	
	vector<string> result;

	size_t string_len = original.size();
	size_t current_offset = 0;
	size_t separator_offset;
	size_t separator_len;

	while (current_offset < string_len) {
	
		separator_offset = original.size();
		separator_len = 0;

		for (const auto& separator : separators) {
			size_t current_find = original.find(separator, current_offset);

			// no separator found; go on to the next separator
			if (current_find == string::npos) {
				continue;

			// if a separator was found at an earlier position, favor this
			} else if (current_find < separator_offset) {
				separator_offset = current_find;
				separator_len = separator.size();

			// if two different separators were found at the same position, favor the longer matching
			// separator
			} else if (current_find == separator_offset && separator_len < separator.size()) {
				separator_len = separator.size();
			}
		}

		// there was valid content between separators
		if (current_offset < separator_offset) {
			string token = original.substr(current_offset, separator_offset-current_offset);
			result.push_back(token);
		}

		// advance to the first position after the separator
		current_offset = separator_offset + separator_len;
	}

	return result;
}

// tokenizes a string into key/value pairs. ignores whitespace.
vector<pair<string, string>> string_utils::generate_key_value_pair(const string& input) {

	vector<pair<string, string>> result;
	
	// tokenize by line breaks
	set<string> line_separators;
	line_separators.insert("\n");
	line_separators.insert("\r\n");
	line_separators.insert("\r");
	vector<string> lines = tokenize(input, line_separators);

	// for each line, trim front and back whitespace
	for (auto& line : lines) {
		line = trim_leading_and_trailing_whitespace(line);
	
		// make sure there is only one '=', unless escaped by double quotes
		bool escaped = false;
		bool found_equal = false;
		bool error = false;
		size_t offset = 0;
		size_t line_len = line.size();
		for (uint32_t counter = 0; counter < line_len; ++counter) {
			switch (line[counter]) {
				case '"':
					escaped = !escaped;
					break;
				case '=':
					if (!escaped) {
						if (found_equal) {
							error = true;
						} else {
							found_equal = true;
							offset = counter;
						}
					}
					break;
				default:
					break;
			}
		}

		// sanity check
		if (error || !found_equal) continue;

		// read content
		string key = trim_leading_and_trailing_whitespace(line.substr(0, offset));
		string value = trim_leading_and_trailing_whitespace(line.substr(offset+1));
		if (key.empty() || value.empty()) continue;

		// add to result
		pair<string, string> item(key, value);
		result.push_back(item);
	}

	return result;
}

// removes whitespaces (space, tabs, newlines, carriage returns) before and after actual string content
string string_utils::trim_leading_and_trailing_whitespace(const string& input) {
	std::string result;	
	
	if (input.empty()) return result;

	uint32_t len = input.size();
	uint32_t offset_start=0;
	uint32_t offset_end = input.size()-1;

	// remove leading whitespace
	for (int counter = 0; counter < (int)len; ++counter) {
		if (is_whitespace(input[counter])) {
			offset_start = counter+1;
		} else {
			break;
		}
	}

	// remove trailing whitespace
	for (int counter = len-1; counter >= 0; --counter) {
		if (is_whitespace(input[counter])) {
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

// checks if a candidate character is whitespace or not
bool string_utils::is_whitespace(char input) {
	if (input == ' ' || input == '\t' || input == '\n' || input == '\r') {
		return true;
	}
	return false;
}

// checks if a candidate character is a number or letter. does not include other printable characters,
// such as spaces, tabs and symbols!
bool string_utils::is_alphanumeric(char input) {
	if ((input >= '0' && input <= '9') || (input >= 'a' && input <= 'z') || (input >= 'A' && input <= 'Z')) {
		return true;
	}
	return false;
}

// attempts to parse a string into an unsigned 32 bit integer
bool string_utils::string_to_uint32(const string& input, uint32_t* storage) {
	return sscanf(input.c_str(), "%u", storage) == 1;
}

// attempts to parse a string into an unsigned 16 bit integer
bool string_utils::string_to_uint16(const string& input, uint16_t* storage) {
	return sscanf(input.c_str(), "%hu", storage) == 1;
}

// attempts to parse a string into a signed integer
bool string_utils::string_to_int(const string& input, int* storage) {
	return sscanf(input.c_str(), "%d", storage) == 1;
}
