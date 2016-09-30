#include <arpa/inet.h>
#include "autobuf_packer.h"

// removes all buffers
void autobuf_packer::clear() {
	buffers.clear();
}

// appends one single buffer. makes a full copy
void autobuf_packer::add(const autobuf& buf) {
	buffers.push_back(buf.copy_as_owned());
}

// appends an entire vector of buffers
void autobuf_packer::add(const vector<autobuf>& bufs) {
	for (const auto& buf : bufs) {
		add(buf);
	}
}

// serializes all buffers into one autobuf
uint32_t autobuf_packer::serialize(autobuf& dest) const {

	// calculate size needed (one root metadata unit + n entry metadata + autobufs)
	uint32_t size_needed = sizeof(autobuf_packer_info_t) + buffers.size()*sizeof(autobuf_entry_t);
	for (const auto& buf : buffers) {
		size_needed += buf.size();
	}

	// create buffer
	dest.create_empty_buffer(size_needed);
	autobuf_packer_info_t* ptr = (autobuf_packer_info_t*) dest.get_content_ptr_mutable();

	// write root metadata
	uint32_t size_needed_net = htonl(size_needed);
	memcpy(ptr->num_entries, &size_needed_net, sizeof(size_needed_net));

	// write autobuf metadata and autobuf contents
	uint32_t offset = 0;
	for (const auto& buf : buffers) {

		// setup per-entry metadata
		autobuf_entry_t entry;
		uint32_t size_net = htonl(buf.size());
		memcpy(entry.size, &size_net, sizeof(uint32_t));

		// copy in metadata and actual content
		memcpy(ptr->packed_content+offset, &entry, sizeof(entry));
		offset += sizeof(entry);
		memcpy(ptr->packed_content+offset, buf.get_content_ptr(), buf.size());
		offset += buf.size();
	}

	return size_needed;
}

// writes vector of buffers onto disk
bool autobuf_packer::serialize_to_disk(const string& filename) const {
	autobuf dest;
	serialize(dest);
	return dest.save(filename);
}

// deserializes from a buffer into the internal object
bool autobuf_packer::deserialize(const autobuf& dest) {
	clear();

	// load root metadata
	const autobuf_packer_info_t* ptr = (const autobuf_packer_info_t*) dest.get_content_ptr();
	if (dest.size() < sizeof(autobuf_packer_info_t)) {
		printf("autobuf_packer::deserialize() error -- unable to parse root metadata (it's too small).\n");
		return false;
	}

	uint32_t packed_num_entries = *((uint32_t*) ptr->num_entries);
	uint32_t num_entries = ntohl(packed_num_entries);
	uint8_t* end_ptr = ((uint8_t*)dest.get_content_ptr()) + dest.size();	// first invalid byte

	// load each entry
	uint32_t offset = 0;
	for (uint32_t counter = 0; counter < num_entries; ++counter) {
		const autobuf_entry_t* entry = (const autobuf_entry_t*)(ptr->packed_content+offset);

		// read out size
		if (entry->packed_content-1 >= end_ptr) {
			printf("autobuf_packer::deserialize() error -- unable to parse entry metadata (it's too small).\n");
			return false;
		}
		uint32_t packed_size = *((uint32_t*)entry->size);
		uint32_t size = ntohl(packed_size);

		// handle empty readouts separately
		if (size == 0) {
			buffers.push_back(autobuf());
		} else {

			// make sure readable content is within valid range
			if (entry->packed_content + size > end_ptr) {
				printf("autobuf_packer::deserialize() error -- unable to parse entry content.\n");
				return false;
			}
		
			autobuf buf;
			buf.set_content(entry->packed_content, size);
			buffers.push_back(move(buf));
		}

		offset += sizeof(autobuf_entry_t) + size;
	}

	if (buffers.size() != num_entries || ptr->packed_content+offset != end_ptr) {
		printf("autobuf_packer::deserialize() error -- deserialization incomplete/error.\n");
		printf("num_bufs: %u bufs parsed: %zd, offset: %p end offset %p.\n", num_entries, buffers.size(),
			ptr->packed_content+offset, end_ptr);
	}
	
	return true;
}

// deserializes from a disk into the internal object
bool autobuf_packer::deserialize_from_disk(const string& filename) {
	autobuf buf;
	if (buf.load(filename)) {
		return deserialize(buf);
	} else {
		return false;
	}
}

// returns the stored buffers
vector<autobuf> autobuf_packer::get_autobufs() const {
	return buffers;
}
