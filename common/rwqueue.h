#ifndef __RW_QUEUE
#define __RW_QUEUE

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <stdint.h>

using namespace std;
template <class T> class rwqueue {
public:

	// constructors and assignment operator
	rwqueue(int max=-1):max_(max) {}
	rwqueue(const rwqueue& other):max_(other.max_) {}
	rwqueue& operator=(const rwqueue& other) {
		if (&other == this) {
			return *this;
		} else {
			max_ = other.max_;
			obj_queue_ = other.obj_queue_;
			return *this;
		}
	}

	// clears the rwqueue object
	void clear() {
		unique_lock<mutex> g(lock_, defer_lock);
		g.lock();
		while (obj_queue_.size() > 0) {
			obj_queue_.pop();
		}
//		obj_queue_.clear();
		full_cond_.notify_all();
		max_ = -1;
		g.unlock();
	}

	// enqueues an object by moved reference
	void enqueue(T&& obj) {
		unique_lock<mutex> g(lock_, defer_lock);
		g.lock();
		while (max_ >= 0 && obj_queue_.size() >= max_) {
			full_cond_.wait(g);
		}
		obj_queue_.push(move(obj));
		empty_cond_.notify_all();
		g.unlock();
	}

	// enqueues and object by original object
	void enqueue(const T& obj) {
		unique_lock<mutex> g(lock_, defer_lock);
		g.lock();
		while(max_ >= 0 && obj_queue_.size() >= max_) {
			full_cond_.wait(g);
		}
		obj_queue_.push(obj);
		empty_cond_.notify_all();
		g.unlock();
	}

	// dequeues an object
	T dequeue() {
		unique_lock<mutex> g(lock_, defer_lock);
		g.lock();
		while (obj_queue_.empty()) {
			empty_cond_.wait(g);
		}
		T result = move(obj_queue_.front());
		obj_queue_.pop();
		full_cond_.notify_all();
		g.unlock();

		return result;
	}

	// dequeues an object, with timeout
	bool dequeue_with_timeout(T& result, uint32_t timeout_ms) {
		unique_lock<mutex> g(lock_, defer_lock);
		g.lock();
		if (empty_cond_.wait_for(g, chrono::milliseconds(timeout_ms), [this]() { return !obj_queue_.empty(); } ) == cv_status::timeout) {
			g.unlock();
			return false;
		} else {
			result = move(obj_queue_.front());
			obj_queue_.pop();
			full_cond_.notify_all();
			g.unlock();
			return true;
		}
	}

	// sets the maximum queue size
	void set_max(int max) {
		unique_lock<mutex> g(lock_, defer_lock);
		g.lock();
		max_ = (max >= -1 ? max : -1);
		full_cond_.notify_all();
	}

	// gets the maximum queue size
	int get_max() const {
		lock_guard<mutex> g(lock_);
		return max_;
	}

	// gets the size of the queue
	uint32_t size() const {
		lock_guard<mutex> g(lock_);
		return obj_queue_.size();
	}

	// tells if the queue is empty
	bool is_empty() const {
		lock_guard<mutex> g(lock_);
		return obj_queue_.empty();
	}

private:

	mutable mutex lock_;
	mutable condition_variable full_cond_;
	mutable condition_variable empty_cond_;
	uint32_t max_;
	queue<T> obj_queue_;

};

#endif
