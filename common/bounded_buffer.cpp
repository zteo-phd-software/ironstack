#include "bounded_buffer.h"

// constructor
bounded_buffer::bounded_buffer() {
	max_size = 32768;
}

// constructor with max size
bounded_buffer::bounded_buffer(uint32_t max) {
	max_size = max;
}

// destructor
bounded_buffer::~bounded_buffer() {
	std::unique_lock<std::mutex> g(lock);
	g.lock();
	buf.clear();
	full_cond.notify_all();
	empty_cond.notify_all();
	g.unlock();
	g.lock();
	g.unlock();
}

// clears the buffer
void bounded_buffer::clear() {
	std::lock_guard<std::mutex> g(lock);
	buf.clear();
}

// enqueues a buffer, blocking if the buffer has reached capacity
void bounded_buffer::enqueue(const autobuf& incoming_buf) {
	std::unique_lock<std::mutex> g(lock, std::defer_lock);
	g.lock();
	while (max_size > 0 && buf.size() + incoming_buf.size() >= max_size) {
		full_cond.wait(g);
	}
	buf += incoming_buf;
	empty_cond.notify_all();
	g.unlock();

}

// enqueue a buffer regardless of its existing size
void bounded_buffer::force_enqueue(const autobuf& incoming_buf) {
	std::unique_lock<std::mutex> g(lock, std::defer_lock);
	g.lock();
	buf += incoming_buf;
	empty_cond.notify_all();
	g.unlock();
}

// dequeues up to a certain number of bytes
autobuf bounded_buffer::dequeue(uint32_t max_read_size) {
	autobuf result;
	uint32_t read_size;
	std::unique_lock<std::mutex> g(lock, std::defer_lock);
	g.lock();
	while (buf.size() == 0) {
		empty_cond.wait(g);
	}
	read_size = (buf.size() >= max_read_size ? max_read_size : buf.size());
	buf.read_front(result, read_size);
	buf.trim_front(read_size);
	g.unlock();

	return std::move(result);
}

// dequeues an exact number of bytes from a buffer
autobuf bounded_buffer::dequeue_exact(uint32_t read_size) {
	autobuf result;
	
	std::unique_lock<std::mutex> g(lock, std::defer_lock);
	g.lock();
	while(buf.size() < read_size) {
		empty_cond.wait(g);
	}
	buf.read_front(result, read_size);
	buf.trim_front(read_size);
	g.unlock();

	return std::move(result);
}

// returns the current size of the buffer
uint32_t bounded_buffer::size() const {
	std::lock_guard<std::mutex> g(lock);
	return buf.size();
}

// returns the maximum buffer size
uint32_t bounded_buffer::get_max() const {
	std::lock_guard<std::mutex> g(lock);
	return max_size;
}

// sets the maximum size of the bounded buffer,
// potentially waking up all blocked writer threads
void bounded_buffer::set_max(uint32_t max) {
	std::unique_lock<std::mutex> g(lock, std::defer_lock);
	g.lock();
	if (max_size != max) {
		max_size = max;
		full_cond.notify_all();
	}
	g.unlock();
}
