#ifndef __BOUNDED_BUFFER
#define __BOUNDED_BUFFER

#include <condition_variable>
#include <mutex>
#include <thread>
#include "autobuf.h"

// provides an implementation of a bounded buffer
// writer blocks when the buffer has reached a specified size limit

class bounded_buffer {
public:

	bounded_buffer();
	bounded_buffer(uint32_t max);
	~bounded_buffer();
	
	void clear();
	void enqueue(const autobuf& buf);
	void force_enqueue(const autobuf& buf);
	autobuf dequeue(uint32_t max_read_size);
	autobuf dequeue_exact(uint32_t read_size);

	uint32_t size() const;
	uint32_t get_max() const;
	void set_max(uint32_t max);

private:

	mutable std::mutex lock;
	std::condition_variable full_cond;
	std::condition_variable empty_cond;

	uint32_t max_size;
	autobuf buf;

};


#endif
