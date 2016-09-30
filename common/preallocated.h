#ifndef __PREALLOCATED_OBJECT_POOL
#define __PREALLOCATED_OBJECT_POOL

#include <list>
#include <mutex>
#include <stdint.h>

template <class T> class preallocated {
public:

	// constructor
	preallocated(int initial=0, bool allow_alloc_=false):allow_alloc(allow_alloc_) {
		#ifndef __MEMORY_DEBUGGING
		for (int counter = 0; counter < initial; ++counter) {
			reusables.push_back(new T());
		}
		#endif
	}

	// destructor
	~preallocated() {
		#ifndef __MEMORY_DEBUGGING
		clear();
		#endif
	}

	// allow reallocation
	void allow_allocation(bool flag) {
		#ifndef __MEMORY_DEBUGGING
		allow_alloc = flag;
		#endif
	}

	// releases memory held by all reusable objects that are
	// currently available in the pool (note: does not free up
	// objects in use!)
	void clear() {
		#ifndef __MEMORY_DEBUGGING
		std::lock_guard<std::mutex> g(lock);
		for (const auto& it : reusables) {
			delete it;
		}
		reusables.clear();
		#endif
	};

	// returns the next available element if pool has capacity
	// otherwise alloc or return nullptr
	T* allocate() {
		#ifndef __MEMORY_DEBUGGING
		std::lock_guard<std::mutex> g(lock);
		if (!reusables.empty()) {
			T* result = reusables.front();
			reusables.pop_front();
			return result;
		} else if (allow_alloc) {
			return new T();
		}
		return nullptr;
		#else
			return new T();
		#endif
	}

	// add more elements into the pool
	void grow_allocation(int units) {
		#ifndef __MEMORY_DEBUGGING
		std::lock_guard<std::mutex> g(lock);
		for (int counter = 0; counter < units; ++counter) {
			reusables.push_back(new T());
		}
		#endif
	}

	// returns number of free elements left
	uint32_t get_free_capacity() const {
		#ifndef __MEMORY_DEBUGGING
		std::lock_guard<std::mutex> g(lock);
		return reusables.size();
		#else
		return (uint32_t)-1;
		#endif
	}

	// returns an object to the pool
	void free(T* ptr) {
		#ifndef __MEMORY_DEBUGGING
		std::lock_guard<std::mutex> g(lock);
		reusables.push_back(ptr);
		#else
		delete ptr;
		#endif
	}

private:

	#ifndef __MEMORY_DEBUGGING
	mutable std::mutex lock;
	bool allow_alloc;
	std::list<T*> reusables;
	#else
	bool allow_alloc;
	#endif
};


#endif
