#pragma once

/*
 * this class provides some useful string utilities.
 *
 */
#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <sstream>

using namespace std;
namespace string_utils {

	// tokenizes a string by separator units.
	vector<string> tokenize(const string& original, const set<string>& separators);
	vector<string> tokenize(const string& original, const string& separator);
	vector<string> tokenize(const string& original, const char* separator);
	vector<string> tokenize(const string& original, char separator);

	// tokenizes a string into key/value pairs. key/value pairs are defined as "x=y" in the original string, and are separated
	// by tokens as defined in separators
	// prunes whitespace.
	vector<pair<string, string>> generate_key_value_pair(const string& input);
	
	// removes leading and trailing whitespace in a string
	string trim_leading_and_trailing_whitespace(const string& input);

	// checks a candidate character to see if it is whitespace or alphanumeric (digits/letters)
	bool is_whitespace(char input);
	bool is_alphanumeric(char input);

	// some conversion utilities
	bool string_to_uint32(const string& input, uint32_t* storage);
	bool string_to_uint16(const string& input, uint16_t* storage);
	bool string_to_int(const string& input, int* storage);

	// converts a string to some templated numeric data type
	template<class T> string number_to_string(T number) {
		stringstream ss;
		ss << number;
		return ss.str();
	}

};
