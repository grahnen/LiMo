#ifndef REGISTER_MONITOR_H
#define REGISTER_MONITOR_H

#include "interval.h"
#include "monitor.hpp"
#include "typedef.h"
#include "util.h"

struct RegValue 
{
	timestamp_t write_call = 0;
	timestamp_t write_ret = 0;
	timestamp_t min_read_call = LLONG_MAX;
	timestamp_t max_read_call = 0;
	timestamp_t min_read_ret = LLONG_MAX;
	timestamp_t max_read_ret = 0;
};

class RegisterMonitor : public Monitor {
protected:
	using LinRes = LinearizationResult<bool>;
	bool unlin = false;
	std::map<val_t, RegValue> values;
	std::map<tid_t, event_t> active;
	std::vector<timestamp_t> times;

	void add_time(timestamp_t t)
	{
		if(times.size() == 0 || times.back() != t)
		{
			times.push_back(t);
		}
	}

	DECLHANDLER(write);
	DECLHANDLER(read);
	// DECLHANDLER(push); // push <-> write
	// DECLHANDLER(pop);  // pop  <-> read 

	void handle_crash(event_t& e);

public:

	void do_linearization();
	void print_state() const;

	RegisterMonitor(MonitorConfig mc);
};




#endif