#ifndef STACK_UNKNOWN_MONITOR_H
#define STACK_UNKNOWN_MONITOR_H

#include "history.h"
#include "stackUnknownHistory.hpp"
#include "monitor.hpp"
#include "typedef.h"

class StackUnknownMonitor : public Monitor
{
protected:
	StackUnknownHistoryAfter history;
	
	DECLHANDLER(push)
	DECLHANDLER(pop)
	void handle_crash(event_t &);

public:
	void do_linearization();
	StackUnknownMonitor(MonitorConfig config);

	void print_state() const
	{
		history.print_state();
	}
	
	bool ADT_supported(ADT adt) {return adt == ADT::stack; } 

};

#endif //STACK_UNKNOWN_MONITOR_H