#pragma once

#include <chrono>
#include <thread>
using namespace std;

// an improved timer class to track time displacement and
// provide sleep functions
class timer {
public:

	timer();

	// sets the timer to the current time and clears the deadline
	// if any threads were sleeping on the sleep_until_timer_deadline()
	// function, they will continue to sleep
	void   clear();

	// same as clear(), but makes the code easier to read in some places
	void   reset();

	// sets the time point to now. 
	void   start();

	// sets the time point to now + displacement (which can be negative)
	void   set_start_relative_to_now(int displacement_ms);
	void   set_deadline_ms(int deadline_ms);

	void   sleep_until_timer_deadline(int deadline_ms=-1);
	int    get_time_elapsed_ms() const;	// TODO -- should actually be uint32_t. maybe this was to account for early-late timestamps?

	static void     sleep_for_ms(int interval_ms);
	static uint32_t ms_since_epoch();

private:

	chrono::steady_clock::time_point last_;
	uint32_t                         deadline_ms_; // always measured from the
                                                 // start time point
};

