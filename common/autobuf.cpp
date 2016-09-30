#include "autobuf.h"

// default constructor
autobuf::autobuf() {
	init();
}

// constructor that copies a given buffer
autobuf::autobuf(const void* src_buf, uint32_t size) {
	init();
	set_content(src_buf, size);
}

// copy constructor
autobuf::autobuf(const autobuf& original) {
	init();
	constructor_copy(original);
}

// assignment operator
autobuf& autobuf::operator=(const autobuf& original) {
	if (&original == this) {
		return *this;
	}

	reset();
	constructor_copy(original);
	return *this;
}

// destructor
autobuf::~autobuf() {
	reset();
}

// make a complete copy of this autobuf
autobuf autobuf::copy_as_owned() const {
	autobuf result;
	result.set_content(get_content_ptr(), size());
	return result;
}

// make a shared copy of this autobuf
autobuf autobuf::copy_as_shared() {
	autobuf result;
	result.owns_memory = false;
	result.writeable_buf = get_content_ptr_mutable();
	result.content_size = size();
	result.buf_size = reserve_size();
	return result;
}

// makes a read only copy of this autobuf
autobuf autobuf::copy_as_read_only() const {
	autobuf result;
	result.owns_memory = false;
	result.read_only = true;
	result.read_only_buf = get_content_ptr();
	result.content_size = content_size;
	return result;
}

// makes a complete, owned copy of this autobuf to another autobuf
void autobuf::full_copy_to(autobuf& dest) const {
	dest.reset();
	dest.set_content(get_content_ptr(), size());
}

// makes a complete, owned copy of this autobuf from another autobuf
void autobuf::full_copy_from(const autobuf& src) {
	reset();
	set_content(src.get_content_ptr(), src.size());
}

// comparison operators
bool autobuf::operator==(const autobuf& other) const {
	if (content_size != other.content_size ||
		memcmp(get_content_ptr(), other.get_content_ptr(), content_size) != 0) {
		return false;
	}
	return true;
}

bool autobuf::operator!=(const autobuf& other) const {
	return !(*this == other);
}

// resets the buffer without deallocating resources
void autobuf::clear() {
	if (read_only) {
		printf("autobuf::clear() error -- cannot clear() a read-only buffer!\n");
		abort();
	} else {
		content_size = 0;
	}
}

// deallocates all resources and resets the object to the uninitialized state
void autobuf::reset() {
	if (owns_memory && writeable_buf != nullptr) {
		free(writeable_buf);
	}
	init();
}

// updates the contents of the writeable buffer
void autobuf::set_content(const void* src_buf, uint32_t size) {
	if (read_only) {
		printf("autobuf::set_content() error -- buffer is read-only!\n");
		abort();
	} else {
		if (!alloc(size)) {
			if (owns_memory) {
				printf("autobuf::set_content() error -- shared buffer is not big "
					"enough. requested %u bytes, available %u bytes.\n", size,
					buf_size - begin_offset);
			} else {
				printf("autobuf::set_content() error -- unable to allocate space "
					"for %u bytes.\n", size);
			}
			abort();
		}

		memcpy(get_content_ptr_mutable(), src_buf, size);
		content_size = size;
		begin_offset = 0;
	}
}

// sets the object to point to a read-only buffer
void autobuf::inherit_read_only(const void* src_buf, uint32_t size) {
	reset();
	read_only = true;
	read_only_buf = src_buf;
	content_size = size;
	buf_size = size;
}

// inherits the object as a shared buffer
void autobuf::inherit_shared(void* src_buf, uint32_t size, uint32_t max_size) {
	reset();
	owns_memory = false;
	writeable_buf = src_buf;
	content_size = size;
	buf_size = (max_size == 0 ? size : max_size);
}

// a safer way to copy memory into an autobuf (bounds are checked)
void autobuf::memcpy_from(const void* src_buf, uint32_t size, uint32_t offset) {
	if (read_only) {
		printf("autobuf::memcpy_from() error -- cannot copy into read-only "
			"buffer.\n");
		abort();
	} else {

		if ((uint8_t*) writeable_buf + size + begin_offset + offset >
			(uint8_t*) writeable_buf + buf_size) {
			printf("autobuf::memcpy_from() error -- copy will exceed available "
				"reserve! bytes to copy: %u, bytes available: %u.\n", size,
				buf_size - begin_offset);
				abort();
		}

		memcpy(ptr_offset_mutable(offset), src_buf, size);
	}
}

// a safer way to copy memory from an autobuf (bounds checked on the autobuf)
void autobuf::memcpy_to(void* dst_buf, uint32_t size, uint32_t offset) const {
	const void* src = nullptr;
	if (read_only) {
		src = read_only_buf;
	} else {
		src = writeable_buf;
	}

	if (offset + size > content_size) {
		printf("autobuf::memcpy_to() error -- insufficient bytes to copy. "
			"requested %u bytes, available %u bytes.\n", size, content_size);
		abort();
	} else if (src == nullptr) {
		printf("autobuf::memcpy_to() error -- src buffer is empty.\n");
		abort();
	}

	memcpy(dst_buf, ptr_offset_const(offset), size);
}

// array access operator for the autobuf
uint8_t& autobuf::operator[](int offset) {
	if (offset < 0 || offset >= (int) content_size) {
		printf("autobuf::operator[] range error -- requested offset %d, content "
			"size: %u bytes.\n", offset, content_size);
		abort();
	}

	if (read_only) {
		// return a reference to a dummy char that can be modified -- it will
		// not affect the original read-only buffer.
		dummy_ret = ((uint8_t*)read_only_buf)[begin_offset+offset];
		return dummy_ret;
	}

	return ((uint8_t*)writeable_buf)[begin_offset+offset];
}

// array access operator for a const autobuf
uint8_t autobuf::operator[](int offset) const {
	if (offset < 0 || offset >= (int) content_size) {
		printf("autobuf::operator[] range error -- requested offset %d, content "
			"size: %u bytes.\n", offset, content_size);
		abort();
	}

	if (read_only) {
		return ((const uint8_t*)read_only_buf)[begin_offset+offset];
	}

	return ((const uint8_t*)writeable_buf)[begin_offset+offset];
}


// reads the front few bytes of the autobuf into a primitive type array
void autobuf::read_front(void* dest, uint32_t len) const {
	if (len > content_size) {
		printf("autobuf::read_front() error -- range error. requested %u bytes, "
			"%u bytes available.\n", len, content_size);
		abort();
	}
	memcpy(dest, get_content_ptr(), len);
}

// reads the rear few bytes of the autobuf into a primitive type array
void autobuf::read_rear(void* dest, uint32_t len) const {
	if (len > content_size) {
		printf("autobuf::read_rear() error -- range error. requested %u bytes, "
			"%u bytes available.\n", len, content_size);
		abort();
	}
	memcpy(dest, ptr_offset_const(content_size - len), len);
}

// reads a range of bytes into a primitive type array
void autobuf::read(void* dest, uint32_t offset, uint32_t len) const {
	if (offset + len > content_size) {
		printf("autobuf::read() error -- range error. requested %u bytes from "
			"offset %u, only %u bytes available.\n", len, offset,
			content_size - offset);
		abort();
	}
	memcpy(dest, ptr_offset_const(offset), len);
}

// reads the front few bytes of the autobuf into another autobuf
void autobuf::read_front(autobuf& dest, uint32_t len) const {
	if (len > content_size) {
		printf("autobuf::read_front() error -- range error. requested %u bytes, "
			"%u bytes available.\n", len, content_size);
		abort();
	}
	dest.set_content(get_content_ptr(), len);
}

// reads the rear few bytes of the autobuf into another autobuf
void autobuf::read_rear(autobuf& dest, uint32_t len) const {
	if (len > content_size) {
		printf("autobuf::read_rear() error -- range error. requested %u bytes, "
			"%u bytes available.\n", len, content_size);
		abort();
	}
	dest.set_content((const uint8_t*)ptr_offset_const(content_size) - len,
		len);
}

// reads a range of bytes into another autobuf
void autobuf::read(autobuf& dest, uint32_t offset, uint32_t len) const {
	if (offset + len > content_size) {
		printf("autobuf::read() error -- range error. requested %u bytes from "
			"offset %u, only %u bytes available.\n", len, offset,
			content_size - offset);
		abort();
	}
	dest.set_content((const uint8_t*)ptr_offset_const(offset), len);
}

// returns a const pointer to the offsetted buffer
const void* autobuf::get_content_ptr() const {
	return (const uint8_t*)(read_only ? read_only_buf : writeable_buf) + 
		begin_offset;
}

// returns a const pointer to the offsetted buffer, offset by another amount
const void* autobuf::ptr_offset_const(int offset) const {
	if (offset < 0) {
		printf("autobuf::ptr_offset_const() error -- offset %d cannot be "
			"negative.\n", offset);
		abort();
	}
	return (const uint8_t*)(read_only ? read_only_buf : writeable_buf) +
		begin_offset + offset;
}

// returns an offsetted mutable pointer to the content
void* autobuf::get_content_ptr_mutable() {
	if (read_only) {
		printf("autobuf::get_content_ptr_mutable() error -- cannot modify a "
			"read-only buffer.\n");
		abort();
	}
	return (uint8_t*)writeable_buf + begin_offset;
}

// returns an offsetted mutable pointer, offset by another amount
void* autobuf::ptr_offset_mutable(int offset) {
	if (read_only) {
		printf("autobuf::ptr_offset_mutable() error -- cannot modify a read-only "
			"buffer.\n");
		abort();
	} else if (offset < 0 || offset >= (int) content_size) {
		printf("autobuf::ptr_offset_mutable() error -- offset %d out of bounds.\n",
			offset);
		abort();
	}
	return (uint8_t*)writeable_buf + begin_offset + offset;
}

// trims the front part of the buffer
void autobuf::trim_front(uint32_t len) {
	if (len >= content_size) {
		begin_offset = content_size;
		content_size = 0;
	} else {
		begin_offset += len;
		content_size -= len;
	}
}

// trims the rear part of the buffer
void autobuf::trim_rear(uint32_t len) {
	if (len >= content_size) {
		content_size = 0;
		return;
	}

	content_size -= len;
}

// appends a given autobuf
autobuf& autobuf::operator+=(const autobuf& other) {
	if (read_only) {
		printf("autobuf::operator+=() error -- cannot append to read-only "
			"buffer.\n");
		abort();
	}

	// guard against self assignment
	if (this == &other) {
		if (content_size > 0) {
			if (!alloc(content_size << 1)) {
				printf("autobuf::operator+=() error -- unable to allocate space.\n");
				abort();
			}

			// use get_content_ptr() here because ptr_offset_mutable checks bounds
			// and we have not updated buf_size yet.
			memcpy((uint8_t*)get_content_ptr() + content_size, get_content_ptr(),
				content_size);
			content_size <<= 1;
		}
		return *this;
	}

	// special checks for if either buffer is empty
	if (other.content_size == 0) {
		return *this;
	} else if (content_size == 0) {
		set_content(other.get_content_ptr(), other.content_size);
		return *this;
	}

	// neither buffers are empty
	uint32_t total_size = content_size + other.content_size;
	if (!alloc(total_size)) {
		printf("autobuf::operator+=() error -- unable to allocate space.\n");
		abort();
	}

	// need to use get_content_ptr() here because buf_size hasn't been updated
	// yet, and ptr_offset_mutable() checks for bounds.
	memcpy((uint8_t*)get_content_ptr() + content_size, other.get_content_ptr(),
		other.content_size);
	content_size = total_size;

	return *this;
}

// adds two autobufs, creating a new one
autobuf autobuf::operator+(const autobuf& other) const {
	autobuf result;
	result.set_content(get_content_ptr(), content_size);
	result += other;
	return result;
}

// appends a primitive type buffer to the autobuf
void autobuf::append(const void* buf, uint32_t len) {
	if (read_only) {
		printf("autobuf::append() error -- cannot append to a read-only "
			"buffer!\n");
		abort();
	}

	if (len == 0) return;
	alloc(content_size + len);
	memcpy((uint8_t*)get_content_ptr_mutable() + content_size, buf, len);
	content_size += len;
}

// prepends a primitive type buffer to the autobuf
void autobuf::prepend(const void* buf, uint32_t len) {
	if (read_only) {
		printf("autobuf::prepend() error -- cannot append to a read-only "
			"buffer!\n");
		abort();
	}

	if (len == 0) return;
	if (begin_offset >= len) {
		begin_offset -= len;
		memcpy(get_content_ptr_mutable(), buf, len);
		content_size += len;
		return;
	} else {
		if (!alloc(content_size + len)) {
			printf("autobuf::prepend() error -- unable to allocate memory.\n");
			abort();
		}
		memmove(ptr_offset_mutable(len), get_content_ptr(), content_size);
		memcpy(get_content_ptr_mutable(), buf, len);
		content_size += len;
	}
}

// loads a binary from disk into an owned autobuf
bool autobuf::load(const string& filename) {
	if (read_only) {
		printf("autobuf::load() error -- cannot load into read-only buffer!\n");
		abort();
	}

	reset();

	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == nullptr) {
		printf("autobuf::load() error -- unable to open file [%s] for reading.\n",
			filename.c_str());
		return false;
	}

	long file_size = 0;
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	if (file_size == 0) {
		fclose(fp);
		return true;
	}
	fseek(fp, 0, SEEK_SET);

	if (!alloc(file_size)) {
		printf("autobuf::load() error -- failed to allocate memory. file [%s] "
			"required %ld bytes.\n", filename.c_str(), file_size);
		abort();
	}

	if (fread(writeable_buf, 1, file_size, fp) != (size_t) file_size) {
		printf("autobuf::load() error -- could not read file [%s].\n",
			filename.c_str());
		fclose(fp);
		reset();
		return false;
	}

	fclose(fp);
	content_size = file_size;
	return true;
}

// saves an autobuf into disk
bool autobuf::save(const string& filename) const {
	FILE* fp = fopen(filename.c_str(), "wb");
	if (fp == nullptr) {
		printf("autobuf::save() error -- unable to open file [%s] for writing.\n",
			filename.c_str());
		return false;
	}

	if (fwrite(get_content_ptr(), 1, content_size, fp) != content_size) {
		printf("autobuf::save() error -- could not write to file [%s].\n",
			filename.c_str());
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}

// check ownership type
autobuf::ownership autobuf::get_ownership_type() const {
	if (read_only) {
		return autobuf::ownership::READ_ONLY;
	} else if (owns_memory) {
		return autobuf::ownership::OWNED;
	} else {
		return autobuf::ownership::SHARED;
	}
}

// check if memory is read-only
bool autobuf::is_read_only() const {
	return read_only;
}

// check if memory is shared
bool autobuf::is_shared() const {
	return !read_only && !owns_memory;
}

// check if memory is owned
bool autobuf::is_owned() const {
	return !read_only && owns_memory;
}

// conversion to string
string autobuf::to_string() const {
	string result;
	result.reserve(content_size+1);
	for (uint32_t counter = 0; counter < content_size; ++counter) {
		result.push_back(((const char*)ptr_offset_const(counter))[0]);
	}
	return result;
}

// converts into hex dump
string autobuf::to_hex() const {
	string result;
	int leftover = 0;
	char buf[16];
	for (uint32_t counter = 0; counter < content_size; ++counter) {
		sprintf(buf, "%02x ", ((const uint8_t*)ptr_offset_const(counter))[0]);

		leftover = counter % 8;
		if (leftover == 4) {
			result += "   ";
		}

		if (leftover == 0 && counter != 0) {
			result.push_back('\n');
			result += buf;
		} else {
			result += buf;
		}
	}
	return result;
}

// reserves space without createing content
void autobuf::reserve(uint32_t size) {
	alloc(size);
}

// create an empty buffer that is ready to use, optionally zeroing it out
void autobuf::create_empty_buffer(uint32_t size, bool zero) {
	alloc(size);
	content_size = size;
	if (zero) {
		memset(get_content_ptr_mutable(), 0, content_size);
	}
}

// grows a buffer without altering current contents
void autobuf::grow_buffer(uint32_t size, bool zero) {
	uint32_t old_size = content_size;
	create_empty_buffer(size, false);
	if (zero && size > old_size) {
		uint32_t bytes_to_zero = size - old_size;
		memset(ptr_offset_mutable(old_size), 0, bytes_to_zero);
	}
}

// attempts to shrink the buffer allocation, making the allocation tight
void autobuf::shrink() {
	if (read_only) {
		return;
	}

	if (!owns_memory) {
		printf("autobuf::shrink() error -- cannot shrink a shared buffer.\n");
		abort();
	}

	if (content_size == 0) {
		reset();
		return;
	}

	uint32_t number_of_pages = content_size / BUFFER_INCREMENT_SIZE +
		(content_size % BUFFER_INCREMENT_SIZE > 0 ? 1 : 0);
	uint32_t actual_pages = buf_size / BUFFER_INCREMENT_SIZE +
		(buf_size % BUFFER_INCREMENT_SIZE > 0 ? 1 : 0);

	if (number_of_pages > actual_pages) {
		if (begin_offset > 0) {
			memmove(writeable_buf, get_content_ptr(), content_size);
			begin_offset = 0;
		}
		void* new_ptr = nullptr;
		uint32_t total_bytes = number_of_pages * BUFFER_INCREMENT_SIZE;
		new_ptr = realloc(writeable_buf, total_bytes);
		if (new_ptr == nullptr) {
			printf("autobuf::shrink() error -- failed to allocate memory.\n");
			abort();
		}
		buf_size = total_bytes;
	}
}

// gets the content size
uint32_t autobuf::size() const {
	return content_size;
}

// gets the amount of actual memory used
uint32_t autobuf::reserve_size() const {
	if (read_only) {
		return 0;
	}
	return buf_size;
}

// returns the amount of slack that the buffer can grow to.
uint32_t autobuf::slack_size() const {
	if (read_only) {
		return 0;
	}
	return buf_size - content_size;
}

// used to initialize all variables
void autobuf::init() {
	read_only = false;
	read_only_buf = nullptr;

	owns_memory = true;
	writeable_buf = nullptr;

	buf_size = 0;
	content_size = 0;
	begin_offset = 0;
	dummy_ret = 0;
}

// used by copy constructor and assignment operator to perform copies
void autobuf::constructor_copy(const autobuf& original) {

	if (original.owns_memory) {
		// complete copy

		owns_memory = true;
		uint32_t size = original.size();
		alloc(size);
		content_size = size;
		memcpy(writeable_buf, original.get_content_ptr(), size);

	} else if (!original.owns_memory && !original.read_only) {
		// shared copy

		owns_memory = false;
		read_only = false;
		content_size = original.size();
		buf_size = original.buf_size;
		writeable_buf = original.writeable_buf;
		begin_offset = original.begin_offset;

	} else {
		// read only
		
		read_only = true;
		read_only_buf = original.read_only_buf;
		buf_size = original.buf_size;
		content_size = original.content_size;
		begin_offset = original.begin_offset;
	}
}

// used to allocate memory for the autobuf. automatically realigns contents
// if memory is owned and begin_offset is nonzero. does not update content
// size.
bool autobuf::alloc(uint32_t size) {
	if (read_only) {
		printf("autobuf::alloc() failed -- cannot alloc for a read-only "
			"buffer!\n");
		return false;
	}
	
	if (!owns_memory) {

		// shared memory
		if (begin_offset + size > buf_size) {
			printf("autobuf::alloc() failed -- shared buffer is too small.\n");
			return false;
		} else {
			return true;
		}

	} else {

		// move memory contents if possible
		if (begin_offset > 0) {
			memmove(writeable_buf, (uint8_t*)writeable_buf + begin_offset,
				content_size);
			begin_offset = 0;
		}

		// owned memory
		if (begin_offset + size <= buf_size) {
			return true;
		} else {
			void* new_ptr = nullptr;
			uint32_t required_pages = (size / BUFFER_INCREMENT_SIZE) + 
				(size % BUFFER_INCREMENT_SIZE > 0 ? 1 : 0);
			uint32_t required_size = required_pages * BUFFER_INCREMENT_SIZE;
			new_ptr = realloc(writeable_buf, required_size);

			if (new_ptr == nullptr) {
				printf("autobuf::alloc() failed -- could not allocate memory.\n");
				return false;
			}

			writeable_buf = new_ptr;
			buf_size = required_size;
			return true;
		}
	}
}
