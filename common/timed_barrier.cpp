#include "timed_barrier.h"

// static variables
std::mutex timed_barrier::lock;
uint32_t timed_barrier::barrier_counter = 1;
std::map<uint32_t, timed_barrier::barrier_instance*> timed_barrier::barriers;

// creates a barrier, doesn't start it
uint32_t timed_barrier::create(double interval) {
	barrier_instance* instance = new timed_barrier::barrier_instance();
    instance->milliseconds = interval * 1000;
	std::lock_guard<std::mutex> g(lock);
	barriers[++barrier_counter] = instance;
	return barrier_counter;
}

// begins waiting on the barrier, with a timeout
// returns true if barrier was invalid or if notify was called before timeout
// returns false if barrier was valid and timeout occurred
bool timed_barrier::wait(uint32_t barrier_id) {
	timed_barrier::barrier_instance* instance_ptr = nullptr;
	{
		std::lock_guard<std::mutex> g(lock);
		if (barriers.count(barrier_id) == 0) {
			return true;
		}

		instance_ptr = barriers[barrier_id];
	}

	std::unique_lock<std::mutex> lock(instance_ptr->lock, std::defer_lock);
	lock.lock();
	while (!instance_ptr->activated) {
		if (instance_ptr->cond.wait_for(lock, std::chrono::milliseconds(instance_ptr->milliseconds)) == std::cv_status::timeout) {
			// timed out, return false
			lock.unlock();
			return false;
		}
	}

	lock.unlock();
	return true;
}

// notifies the barrier so that the thread is unblocked (causes wait to return true)
void timed_barrier::notify(uint32_t barrier_id) {
	timed_barrier::barrier_instance* instance_ptr = nullptr;
	std::lock_guard<std::mutex> g(lock);
	if (barriers.count(barrier_id) == 0) {
		return;
	}

	instance_ptr = barriers[barrier_id];

	std::unique_lock<std::mutex> lock(instance_ptr->lock, std::defer_lock);
	lock.lock();
	instance_ptr->cond.notify_all();
	lock.unlock();
}

// resets the barrier so it can be used again
bool timed_barrier::reset(uint32_t barrier_id) {
	timed_barrier::barrier_instance* instance_ptr = nullptr;
	{
		std::lock_guard<std::mutex> g(lock);
		if (barriers.count(barrier_id) == 0) {
			return false;
		}
		instance_ptr = barriers[barrier_id];
	}

	std::unique_lock<std::mutex> lock(instance_ptr->lock, std::defer_lock);
	lock.lock();
	barriers[barrier_id]->activated = false;
	lock.unlock();

	return true;
}

// destroys the barrier (subsequent waits return immediately with true, other barrier calls are no-ops)
void timed_barrier::destroy(uint32_t barrier_id) {
	std::lock_guard<std::mutex> g(lock);
	if (barriers.count(barrier_id) == 1) {
		delete barriers[barrier_id];
		barriers.erase(barrier_id);
	}
}

