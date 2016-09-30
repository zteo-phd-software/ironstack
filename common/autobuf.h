#pragma once

/*
 * automatic memory buffer, reimplemented for speed and better functionality.
 */

#include <string>
#include <stdint.h>
#include <memory.h>

using namespace std;
class autobuf {
public:

	// defines the memory ownership of the autobuf
	// owned    : buffer was dynamically allocated and needs to be freed
	// shared   : buffer is writeable but may not be flexibly extended or shrunk.
	//            someone else owns the buffer so this is not freed in the dtor.
	// read-only: buffer cannot be modified, extended or shrunk. someone else
	//            owns the buffer.
	enum class ownership { READ_ONLY, SHARED, OWNED };

	// various constructors and destructors
	autobuf();
	autobuf(const void* src_buf, uint32_t size);
	autobuf(const autobuf& original);
	autobuf& operator=(const autobuf& original);
	~autobuf();

	// various copy functions
	autobuf copy_as_owned() const;
	autobuf copy_as_shared();
	autobuf copy_as_read_only() const;

	void full_copy_to(autobuf& dest) const;
	void full_copy_from(const autobuf& src);

	// comparators (only checks the contents; ignores ownership checks)
	bool operator==(const autobuf& other) const;
	bool operator!=(const autobuf& other) const;

	// buffer setup
	void clear();		// resets the buffer counters (does not dealloc/resize)
	void reset();		// resets the autobuf (deallocates resources and starts anew)
	void set_content(const void* src_buf, uint32_t size);
	void inherit_read_only(const void* src_buf, uint32_t size);
	void inherit_shared(void* src_buf, uint32_t size, uint32_t max_size=0);

	// copies from a source into this autobuf, but does not resize the autobuf.
	// to copy (and possibly resize the autobuf), use set_content()
	void memcpy_from(const void* src_buf, uint32_t size, uint32_t offset=0);

	// copies to a destination autobuf, checking bounds on the autobuf to make
	// sure the bounds are not overstepped. this is safer than getting a const
	// pointer from get_content_ptr() and then manually invoking memcpy.
	void memcpy_to(void* dest_buf, uint32_t size, uint32_t offset=0) const;

	// buffer read methods
	uint8_t& operator[] (int offset);
	uint8_t operator[] (int offset) const;

	void read_front(void* dest, uint32_t len) const;
	void read_rear(void* dest, uint32_t len) const;
	void read(void* dest, uint32_t offset, uint32_t len) const;

	void read_front(autobuf& dest, uint32_t len) const;
	void read_rear(autobuf& dest, uint32_t len) const;
	void read(autobuf& dest, uint32_t offset, uint32_t len) const;

	// raw buffer manipulation
	const void* get_content_ptr() const;
	const void* ptr_offset_const(int offset) const;
	void* get_content_ptr_mutable();
	void* ptr_offset_mutable(int offset);

	
	// buffer trimming
	void trim_front(uint32_t len);
	void trim_rear(uint32_t len);

	// concatenation operators
	autobuf& operator+=(const autobuf& other);
	autobuf  operator+ (const autobuf& other) const;
	void append(const void* buf, uint32_t len);
	void prepend(const void* buf, uint32_t len);

	// file I/O
	bool load(const string& filename);
	bool save(const string& filename) const;

	// check memory ownership
	ownership get_ownership_type() const;
	bool is_read_only() const;
	bool is_shared() const;
	bool is_owned() const;

	// conversion to string
	string to_string() const;
	string to_hex() const;

	/*
	 * advanced use methods
	 */

	// reserves space without creating content (allows for expansion later
	// without requiring another realloc).
	void reserve(uint32_t size);

	// creates an empty buffer with some given size (buffer can later be shrunk
	// without changing the reserve size). useful when a buffer of a given size
	// is needed right away. zero flag controls if the memory region should be
	// memset to 0 before use (turn off for higher speed; but less safe).
	void create_empty_buffer(uint32_t size, bool zero=true);

	// grows a buffer to a new size (buffer can later be shrunk without changing
	// reserve size) without altering current contents. zero flag controls if
	// the newly-grown region should be memset to 0 before use.
	void grow_buffer(uint32_t size, bool zero=true);

	// shrinks the buffer down to the absolute minimum to conserve memory use.
	void shrink();

	// gets the content size
	uint32_t size() const;

	// checks the amount of actual memory space occupied. returns 0 for read-
	// only buffers.
	uint32_t reserve_size() const;

	// finds out the amount of slack, or wasted memory in this autobuf (defined
	// as the difference between reserve size and content size). also an
	// indicator of how much the content can grow without hitting a realloc/hard
	// limit (if shared). returns 0 for read-only buffers.
	uint32_t slack_size() const;

private:

	static const uint32_t BUFFER_INCREMENT_SIZE = 4096;

	bool read_only;
	const void* read_only_buf;

	bool owns_memory;
	void* writeable_buf;

	uint32_t buf_size;
	uint32_t content_size;

	uint32_t begin_offset;
	uint8_t dummy_ret;

	// helper functions
	void init();
	void constructor_copy(const autobuf& original);
	bool alloc(uint32_t size);
};
