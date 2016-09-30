#ifndef __COMMON_UTILITIES_OOP
#define __COMMON_UTILITIES_OOP

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#include "common_utils.h"
#include "autobuf.h"

int cu_write_autobuf_to_socket(int socket_id, const autobuf& data);
int cu_read_autobuf_from_socket(int socket_id, autobuf& data);
template <class T> bool cu_write_list_to_file(const std::string& filename, const std::list<T>& item_list);
template <class T> bool cu_read_list_from_file(const std::string& filename, std::list<T>& item_list);

uint32_t calculate_checksum32(const autobuf& buf, uint32_t size);
uint32_t calculate_checksum32(const void* buf, uint32_t size);
uint16_t calculate_checksum16(const autobuf& buf, uint32_t size);
uint16_t calculate_checksum16(const void* buf, uint32_t size);

std::string get_input(const std::string& message="");

// used to compute hashes for enum classes
struct enum_hash {
	template <typename T>
	inline typename std::enable_if<std::is_enum<T>::value, std::size_t>::type
	operator()(T const value) const { return static_cast<std::size_t>(value); }
};

// keeps only the unique items in the vector
template <class T> std::vector<T> vector_unique(const std::vector<T>& vec) {
  bool found;

  std::vector<T> result;
  result.reserve(vec.size());

  for (auto it = vec.begin(); it != vec.end(); ++it) {
    found = false;
    for (auto it2 = result.begin(); it2 != result.end(); ++it2) {
      if (*it == *it2) {
        found = true;
        break;
      }
    }
    if (!found) {
      result.push_back(*it);
    }
  }

  return result;
};

// returns the result of src - to_subtract
template <class T> std::vector<T> vector_subtract(const std::vector<T>& src, const std::vector<T>& to_subtract) {
  std::vector<T> result = src;

  for (auto it = to_subtract.begin(); it != to_subtract.end(); ++it) {
    for (auto it2 = result.begin(); it2 != result.end(); ++it2) {
      if (*it == *it2) {
        result.erase(it2);
        break;
      }
    }
  }

  return result;
};

// returns the result of vec1 intersect vec2
template <class T> std::vector<T> vector_intersect(const std::vector<T>& vec1, const std::vector<T>& vec2) {

  bool found;
  std::vector<T> result;
  result.reserve(vec1.size() > vec2.size() ? vec1.size() : vec2.size());

  std::vector<T> vec1_uniq = vector_unique<T>(vec1);
  std::vector<T> vec2_uniq = vector_unique<T>(vec2);

  for (auto it = vec1_uniq.begin(); it != vec1_uniq.end(); ++it) {
    for (auto it2 = vec2_uniq.begin(); it2 != vec2_uniq.end(); ++it2) {
      if (*it == *it2) {
        result.push_back(*it);
        break;
      }
    }
  }

  return result;
};

// returns the result of taking the (unique) union of vec1 and vec2
template <class T> std::vector<T> vector_union(const std::vector<T>& vec1, const std::vector<T>& vec2) {
  bool found;
  std::vector<T> result = vector_unique<T>(vec1);

  // add unique vec2 items to result
  for (auto it = vec2.begin(); it != vec2.end(); ++it) {
    found = false;
    for (auto it2 = result.begin(); it2 != result.end(); ++it2) {
      if (*it == *it2) {
        found = true;
        break;
      }
    }
    if (!found) {
      result.push_back(*it);
    }
  }

  return result;
};

namespace string_utils {

	// tokenizes a string. any of the given chars in separators qualify as a separator by itself
	std::vector<std::string> tokenize(const std::string& original, const std::string& separators);
	std::vector<std::string> tokenize(const std::string& original, char separator);

	// tokenizes a string into key/value pairs. key/value pairs are defined as "x=y" in the original string, and are separated
	// by tokens as defined in separators
	std::vector<std::pair<std::string, std::string> > generate_key_value_pair(const std::string& original, const std::string& separators);

	// tokenizes an input string into key/value pairs. prunes whitespace.
	std::vector<std::pair<std::string, std::string>> generate_key_value_pair(const std::string& input);
	
	// removes leading and trailing whitespace in a string
	std::string trim_leading_and_trailing_whitespace(const string& input);

	bool is_whitespace(char input);
	bool is_alphanumeric(char input);

	bool string_to_uint32(const std::string& input, uint32_t* storage);
	bool string_to_uint16(const std::string& input, uint16_t* storage);
	bool string_to_int(const std::string& input, int* storage);

	// returns bytes used
	uint32_t string_to_constrained_buf(const std::string& input, void* buf, uint32_t buf_size);
	std::string constrained_buf_to_string(const void* buf, uint32_t buf_size);

/*
	std::string number_to_string(uint16_t number) {
		char buf[16];
		sprintf(buf, "%hu", number);
		return std::string(buf);
	}

	std::string number_to_string(uint32_t number) {
		char buf[16];
		sprintf(buf, "%u", number);
		return std::string(buf);
	}

	std::string number_to_string(uint8_t number) {
		char buf[16];
		sprintf(buf, "%u", number);
		return std::string(buf);
	}

	std::string number_to_string(int number) {
		char buf[16];
		sprintf(buf, "%d", number);
		return std::string(buf);
	}
*/

	template<class T> std::string number_to_string(T number) {
		std::stringstream ss;
		ss << number;
		printf("to_string: %s\n", ss.str().c_str());
		return ss.str();
	}

};
#endif
