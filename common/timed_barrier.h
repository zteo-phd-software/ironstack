#ifndef __TIMED_BARRIER
#define __TIMED_BARRIER

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

class timed_barrier {
public:

	static uint32_t create(double interval);
	static bool wait(uint32_t barrier_id);
	static void notify(uint32_t barrier_id);
	static bool reset(uint32_t barrier_id);
	static void destroy(uint32_t barrier_id);

	class barrier_instance {
	public:
		barrier_instance():activated(false),milliseconds(0) {};
		bool activated;
		uint32_t milliseconds;
		std::mutex lock;
		std::condition_variable cond;
	};

private:

	static std::mutex lock;
	static uint32_t barrier_counter;
	static std::map<uint32_t, barrier_instance*> barriers;

};



#endif
