#ifndef STACK_UNKNOWN_MONITOR_H
#define STACK_UNKNOWN_MONITOR_H

#include "history.h"
#include "stackUnknownHistory.hpp"
#include "monitor.hpp"
#include "typedef.h"
#include "segment_tree.hpp"

class StackUnknownMonitor : public Monitor
{
protected:
	StackUnknownHistory history;

	DECLHANDLER(push)
	DECLHANDLER(pop)
	void handle_crash(event_t &);

	timestamp_t findLastValid(const std::map<val_t, CoverVal> &completedValues,
							  const std::set<AtomicInterval> &empties,
							  const AtomicInterval &push,
							  std::set<timestamp_t, std::greater<timestamp_t>> &crashedPops,
							  timestamp_t &max_ts);

	timestamp_t findFirstValid(const std::map<val_t, CoverVal> &completedValues,
							   const std::set<AtomicInterval> &empties,
							   const AtomicInterval &push,
							   std::set<timestamp_t> &crashedPops,
							   timestamp_t &max_ts,
							   std::set<timestamp_t>::iterator begin);

	void CheckStackLin(const CoverHistory &history);

	tsSegmentTree computeCounters();

	// tsSegmentTree counters;

public:
	void do_linearization();
	StackUnknownMonitor(MonitorConfig config);

	void print_state() const
	{
		// history.print_state();
	}

	bool ADT_supported(ADT adt) { return adt == ADT::stack; }
};

#endif // STACK_UNKNOWN_MONITOR_H