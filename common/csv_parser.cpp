#include "csv_parser.h"

// constructor
csv_parser::csv_parser() {
}

// loads a csv file
bool csv_parser::load(const string& filename) {

	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == nullptr) {
		printf("csv_parser::load() failed -- could not open %s.\n", filename.c_str());
		return false;
	}

	// this string stores everything on one line
	vector<string> line;
	string current_line;
	string current_word;
	int num_cols = -1;
	int current_line_size;
	int current_char = -1;
	size_t current_index;
	size_t next_index;

	while(1) {
		line.clear();
		current_line.clear();

		// read a single line in
		while ((current_char = fgetc(fp)) != EOF) {
			if (current_char != '\r' && current_char != '\n') {
				current_line.push_back((char)current_char);
			} else {
				break;
			}
		}

		// parse the line by commas
		current_index = 0;
		current_line_size = current_line.size();
		while (current_line_size > 0 && (int)current_index <= current_line_size) {
			next_index = current_line.find(",", current_index);
			if (next_index == current_index) {
				current_word.clear();
			} else if (next_index != string::npos) {
				current_word = current_line.substr(current_index, next_index-current_index);
			} else {
				current_word = current_line.substr(current_index);
			}
			line.push_back(current_word);
			if (next_index == string::npos) break;
			current_index = next_index+1;
		}

		// sanity checks
		if (line.empty()) {
			if (current_char == '\r' || current_char == '\n') {
				continue;
			} else if (current_char == EOF) {
				break;
			}
		}

		if (num_cols == -1) {
			num_cols = line.size();
		} else if ((int)line.size() != num_cols) {

			printf("csv_parser::load() error -- parsing failed at line %zu. %u columns expected, read %zu columns.\n", contents.size(), num_cols, line.size());
			break;
		} 

		// sanitize the "" within each word
		for (auto& word : line) {
			size_t find_index;
			int word_size = word.size();

			// check if the string was bound in quotes. if so, remove them
			if (word_size >= 2) {
			
				if ((word[0] == '"' && word[word_size-1] != '"') ||
					(word[0] != '"' && word[word_size-1] == '"')) {

					printf("csv_parser::load() error -- parsing failed at line %zu. unmatched \" quotes.\n",
						contents.size());

					goto fail;

				} else if (word[0] == '"' && word[word_size-1] == '"') {
					word.pop_back();
					word.erase(0,1);

					while ((find_index = word.find("\"\"")) != string::npos) {
						word.erase(find_index, 1);
					}
				}
			}
		}

		// done sanitizing, add to contents
		contents.push_back(line);
	}

	return true;

fail:

	clear();
	return false;

}

// saves the csv file
bool csv_parser::save(const string& filename) const {

	FILE* fp = fopen(filename.c_str(), "wb");
	if (fp == nullptr) {
		printf("csv_parser::save() failed -- could not open %s for writing.\n", filename.c_str());
		return false;
	}

	int num_rows = contents.size();
	int num_cols, num_letters;
	char current_char;
	for (int counter = 0; counter < num_rows; ++counter) {

		num_cols = contents[counter].size();
		for (int counter2 = 0; counter2 < num_cols; ++counter2) {
		
			// if the word has quotes inside, make sure to prepend and append quotes to the csv entry
			bool has_quotes = (contents[counter][counter2].find("\"") != string::npos);
			if (has_quotes) {
				fprintf(fp, "\"");
			}

			num_letters = contents[counter][counter2].size();
			for (int counter3 = 0; counter3 < num_letters; ++counter3) {
				current_char = contents[counter][counter2][counter3];
				if (current_char != '"') {
					fprintf(fp, "%c", current_char);
				} else {
					fprintf(fp, "\"\"");
				}
			}

			if (has_quotes) {
				fprintf(fp, "\"");
			}

			if (counter2 != num_cols-1) {
				fprintf(fp, ",");
			}
		}

		fprintf(fp, "\r\n");
	}

	fclose(fp);
	return true;
}

// clears the contents of the csv
void csv_parser::clear() {
	contents.clear();
}

// return the number of rows in the csv file
uint32_t csv_parser::get_rows() const {
	return contents.size();
}

// return the number of columns per row in the csv file
uint32_t csv_parser::get_cols() const {
	if (contents.empty()) {
		return 0;
	} else {
		return contents[0].size();
	}
}

// remove rows that are completely empty
void csv_parser::eliminate_empty_rows() {

	auto iterator = contents.begin();
	while (iterator != contents.end()) {
		bool is_empty = true;
		for (auto iterator2 = iterator->begin(); iterator2 != iterator->end(); ++iterator2) {
			if (!iterator2->empty()) {
				is_empty = false;
				break;
			}
		}

		if (is_empty) {
			iterator = contents.erase(iterator);
		} else {
			++iterator;
		}
	}
}

// remove rows that are not completely filled
void csv_parser::eliminate_non_full_rows() {

	auto iterator = contents.begin();
	while (iterator != contents.end()) {
		bool has_empty = false;
		for (auto iterator2 = iterator->begin(); iterator2 != iterator->end(); ++iterator2) {
			if (iterator2->empty()) {
				has_empty = true;
				break;
			}
		}

		if (has_empty) {
			iterator = contents.erase(iterator);
		} else {
			++iterator;
		}
	}
}

// access a certain row and column
string& csv_parser::operator()(int row, int column) {

	// check if the row and column are within range
	int num_rows = contents.size();
	int num_cols = -1;
	if (row < num_rows) {
		num_cols = contents.size();
		if (column < num_cols) {
			return contents[row][column];
		}
	}

	// clearly the query was out of range. add the rows first if needed
	int rows_needed = row - num_rows +1;
	for (int counter = 0; counter < rows_needed; ++counter) {
		contents.push_back(vector<string>());
	}

	int cols_needed;
	for (auto& line : contents) {
		cols_needed = column - line.size() + 1;
		for (int counter = 0; counter < cols_needed; ++counter) {
			line.push_back(string());
		}
	}

	return contents[row][column];
}
