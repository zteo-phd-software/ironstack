#pragma once
#include <stdint.h>
#include <chrono>
using namespace std;

// used to track history of samples
template<class T> class rate_meter {
public:

	// constructors and destructor
	rate_meter(uint32_t max_samples=100);
	rate_meter(const rate_meter<T>& other);
	rate_meter<T>& operator=(const rate_meter<T>& other);
	~rate_meter();

	// set options
	void     set_history(uint32_t max_samples);

	// read/write functions
	void     reset();
	void     add_sample(T sample);
	T        get_rate(uint32_t history_sec=0) const; // 0 = all recent history
	T        get_cumulative_rate() const;
	T        get_cumulative_count() const;
	uint64_t get_cumulative_time_ms() const;

private:

	// readings and history
	T*                                sample_history;
	chrono::steady_clock::time_point* history_time;
	T                                 cumulative;

	// information
	uint32_t                          samples;
	uint32_t                          max_samples;
	uint32_t                          sample_ptr;
	chrono::steady_clock::time_point  first_sample_time;

	// internal assistive functions
	void copy_from(const rate_meter<T>& other);
};
