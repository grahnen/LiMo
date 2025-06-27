#ifndef Stack_Optimal_MONITOR_H
#define Stack_Optimal_MONITOR_H
#include "monitor.hpp"
#include "coverhistory.h"
#include "typedef.h"

class StackOptimalMonitor : public Monitor
{
protected:
	index_t height = 0;
	using LinRes = CoverHistory::LinRes;
	CoverHistory history;
	DECLHANDLER(push)
	DECLHANDLER(pop)
public:
	void do_linearization();
	StackOptimalMonitor(MonitorConfig mc);
	void print_state() const;
	bool ADT_supported(ADT adt);
};

#endif
