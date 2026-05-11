#include "monitor/registermonitor.hpp"


void RegisterMonitor::handle_write(event_t& e)
{
	if(!e.val)
	{
		unlin = true;
		return;
	}
	val_t v = e.val.value();
	if(!values.contains(e.val.value()))
	{
		values[v] = RegValue();
	}
	values[v].write_call = e.timestamp;

	active[e.thread] = e;
}e

void RegisterMonitor::handle_ret_write(event_t& e)
{
	event_t& write_evt = active[e.thread];
	val_t v = write_evt.val.value();
	values.at(v).write_ret = e.timestamp;
	active.erase(e.thread);
}

void RegisterMonitor::handle_read(event_t& e)
{
	active[e.thread] = e;
}

void RegisterMonitor::handle_ret_read(event_t& e)
{
	event_t& read_evt = active[e.thread];
	val_t v = e.val.value_or(read_evt.val.value_or(0));
}

// void RegisterMonitor::handle