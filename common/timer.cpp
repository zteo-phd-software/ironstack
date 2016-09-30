#include "timer.h"

// constructor
timer::timer() {
	start();
	deadline_ms_ = 0;
}

// sets the current time point and clears the sleep deadline
void timer::clear() {
	start();
	deadline_ms_ = 0;
}

// same as clear, but easier to understand in some code implementations
void timer::reset() {
	clear();
}

// resets the time point to the current time
void timer::start() {
	last_ = chrono::steady_clock::now();
}

// resets the time point to the current time adjusted by some displacement
void timer::set_start_relative_to_now(int displacement_ms) {
	chrono::milliseconds time_shift(displacement_ms);
	last_ = chrono::steady_clock::now() - time_shift;
}

// setup a sleep deadline
void timer::set_deadline_ms(int deadline_ms) {
	deadline_ms_ = deadline_ms;
}

// sleeps a thread until the time is up
void timer::sleep_until_timer_deadline(int deadline_ms) {
	int time_to_sleep = 0;
	if (deadline_ms == -1) {
		time_to_sleep = deadline_ms_ - get_time_elapsed_ms();
	} else {
		time_to_sleep = deadline_ms - get_time_elapsed_ms();
	}

	if (time_to_sleep < 0) {
		return;
	}

	sleep_for_ms(time_to_sleep);
}

// gets the amount of time elapsed in milliseconds
int timer::get_time_elapsed_ms() const {
	chrono::steady_clock::time_point now = chrono::steady_clock::now();
	return (chrono::duration_cast<chrono::milliseconds>(now-last_)).count();
}

// sleeps for a given amount of time
void timer::sleep_for_ms(int interval_ms) {
	this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
}

// gets the number of milliseconds since the epoch
uint32_t timer::ms_since_epoch() {
	chrono::time_point<chrono::system_clock, chrono::milliseconds> now =
		chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now());
	return now.time_since_epoch().count();
}
