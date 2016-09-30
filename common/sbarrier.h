#ifndef __SINGLE_BARRIER
#define __SINGLE_BARRIER

#include <mutex>
#include <thread>

// used for unblocking one thread from another
class sbarrier {
public:

	sbarrier() {
		activated = false;
	}

	void wait() {
		std::unique_lock<std::mutex> g(lock, std::defer_lock);
		g.lock();
		while (!activated) {
			cond.wait(g);
		}
		g.unlock();
	};

	void notify() {
		std::unique_lock<std::mutex> g(lock, std::defer_lock);
		g.lock();
		activated = true;
		cond.notify_all();
		g.unlock();
	}

private:

	bool activated;
	std::mutex lock;
	std::condition_variable cond;

};

#endif
