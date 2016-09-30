#include "rate_meter.h"
using namespace std;

// constructor
template<class T> rate_meter<T>::rate_meter(uint32_t max_samples_) {
	sample_history = nullptr;
	history_time = nullptr;
	cumulative = 0;
	samples = 0;
	max_samples = 0;
	sample_ptr = 0;

	set_history(max_samples_);
}

// copy constructor
template<class T> rate_meter<T>::rate_meter(const rate_meter<T>& other) {
	copy_from(other);
}

// assignment operator
template<class T> rate_meter<T>& rate_meter<T>::operator=(const rate_meter<T>& other) {
	if (&other == this) {
		return *this;
	}

	// clear old state if needed
	if (sample_history != nullptr) {
		delete[] sample_history;
	}
	if (history_time != nullptr) {
		delete[] history_time;
	}

	// copy new state from other meter
	copy_from(other);

	return *this;
}

// setup the rate meter history. clears all history as a result
template<class T> void rate_meter<T>::set_history(uint32_t max_samples_) {

	sample_history = new T[max_samples_];
	history_time = new chrono::steady_clock::time_point[max_samples_];

	// TODO -- convert to nothrow later and use logging for errors
	for (uint32_t counter = 0; counter < max_samples_; ++counter) {
		sample_history[counter] = 0;
	}

	cumulative = 0;
	samples = 0;
	max_samples = max_samples_;
	sample_ptr = 0;
}

// resets the rate meter
template<class T> void rate_meter<T>::reset() {
	for (uint32_t counter = 0; counter < max_samples; ++counter) {
		sample_history[counter] = 0;
	}

	cumulative = 0;
	samples = 0;
	sample_ptr = 0;
}

// adds a sample to the rate meter
template<class T> void rate_meter<T>::add_sample(T sample) {
	if (max_samples == 0) return;

	// update history and write position
	sample_history[sample_ptr] = sample;
	history_time[sample_ptr] = chrono::steady_clock::now();
	sample_ptr = (sample_ptr+1) % max_samples;

	// update count of samples (saturates at max_samples)
	if (samples == 0) {
		first_sample_time = history_time[sample_ptr];
	} else {
		samples = (samples < max_samples ? samples+1 : max_samples);
	}

	cumulative += sample;
}

// get the rate over a certain number of seconds
template<class T> T rate_meter<T>::get_rate(uint32_t history_sec) const {

	chrono::steady_clock::time_point now = chrono::steady_clock::now();
	chrono::steady_clock::time_point earliest, latest;

	// add all samples within the specified history
	T total = 0;
	for (uint32_t counter = 0; counter < samples; ++counter) {
		if (history_sec == 0) {
			total += sample_history[counter];

			// discover earliest and latest timepoints
			if (counter == 0) {
				earliest = history_time[counter];
				latest = history_time[counter];
			} else if (history_time[counter] < earliest) {
				earliest = history_time[counter];
			} else if (history_time[counter] > latest) {
				latest = history_time[counter];
			}
			
		} else if (chrono::duration_cast<chrono::milliseconds>(now - history_time[counter]).count() <= history_sec * 1000) {
			total += sample_history[counter];
		}
	}

	// compute rate over the specified history period
	if (history_sec == 0) {
		uint32_t difference_ms = chrono::duration_cast<chrono::milliseconds>(latest - earliest).count();
		if (difference_ms == 0) {
			return 0;
		} else {
			return (total / difference_ms) * 1000;
		}

	} else {
		return total / history_sec;
	}
}

// gets the cumulative rate over the lifetime of the object
// could yield erroneous results on overflow of the accumulator
template<class T> T rate_meter<T>::get_cumulative_rate() const {

	chrono::steady_clock::time_point now = chrono::steady_clock::now();
	int difference_ms = chrono::duration_cast<chrono::milliseconds>(now - first_sample_time).count();
	if (difference_ms <= 0) {
		return 0;
	} else {
		return (cumulative / difference_ms) * 1000;
	}
}

// returns the cumulative value (as opposed to rate)
template<class T> T rate_meter<T>::get_cumulative_count() const {
	return cumulative;
}

// returns the total amount of time elapsed since object reset/instantiation
template<class T> uint64_t rate_meter<T>::get_cumulative_time_ms() const {
	if (samples == 0) return 0;

	chrono::steady_clock::time_point now = chrono::steady_clock::now();
	uint64_t ms_elapsed = chrono::duration_cast<chrono::milliseconds>(now - first_sample_time).count();
	return ms_elapsed;
}

// used by copy constructor and assignment operator to copy in state
template<class T> void rate_meter<T>::copy_from(const rate_meter<T>& other) {
	set_history(other.max_samples);

	for (uint32_t counter = 0; counter < max_samples; ++counter) {
		sample_history[counter] = other.sample_history[counter];
		history_time[counter] = other.history_time[counter];
	}

	cumulative = other.cumulative;
	samples = other.samples;
	sample_ptr = other.sample_ptr;
	first_sample_time = other.first_sample_time;
}
