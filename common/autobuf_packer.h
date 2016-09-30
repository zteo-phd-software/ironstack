#pragma once
#include "autobuf.h"
#include <vector>
using namespace std;

// a class to serialize a vector of autobufs
class autobuf_packer {
public:
	// clear and add buffers
	void     clear();
	void     add(const autobuf& buf);
	void     add(const vector<autobuf>& bufs);

	// serialization functions
	uint32_t serialize(autobuf& dest) const;
	bool     serialize_to_disk(const string& filename) const;

	// deserialization functions
	bool     deserialize(const autobuf& dest);
	bool     deserialize_from_disk(const string& filename);

	// returns a vector of all the stored autobufs
	vector<autobuf> get_autobufs() const;

private:

	vector<autobuf> buffers;

	// metadata for all entries
	typedef struct {
		uint8_t  num_entries[4];
		uint8_t* packed_content;
	} autobuf_packer_info_t;

	// information about each entry
	typedef struct {
		uint8_t  size[4];
		uint8_t* packed_content;
	} autobuf_entry_t;
};
