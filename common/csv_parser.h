#pragma once
#include <stdio.h>
#include <string>
#include <vector>
using namespace std;

class csv_parser {
public:

	// constructor
	csv_parser();

	// attempts to load or save a csv file
	bool load(const string& filename);
	bool save(const string& filename) const;

	// clears the object
	void clear();


	// returns the number of rows and columns in the spreadsheet
	uint32_t get_rows() const;
	uint32_t get_cols() const;

	// eliminates empty rows and non full rows that may be useless
	void eliminate_empty_rows();
	void eliminate_non_full_rows();

	// allows direct access to the csv contents
	//
	// if a new index is accessed, all intermediate rows and columns
	// are added automatically
	string& operator()(int row, int column);

private:

	vector<vector<string>> contents;

};
