#ifndef Queue_UNKNWON_MONITOR_NAIVE_H
#define Queue_UNKNOWN_MONITOR_NAIVE_H

#include "interval.h"
#include "monitor.hpp"
#include "typedef.h"
#include "util.h"
#include "intervaltree.hpp"
#include "queue_unknownmonitor.hpp"

#include <queue>


class QueueNaiveUnknownMonitor : public Monitor
{
protected:
	using LinRes = LinearizationResult<bool>;

	std::vector<AtomicInterval> outers;
	std::vector<AtomicInterval> inners;

	timestamp_t curEra;
	timestamp_t maxTimestamp;

	// std::map<val_t, AtomicInterval> inner;
	// std::map<val_t, AtomicInterval> outer;

	std::vector<timestamp_t> timestamps;
	std::map<timestamp_t, unsigned long long int> queueCtr;
	std::map<timestamp_t, val_t> opVals;
	std::map<timestamp_t, timestamp_t> opEras;

	std::vector<QueueOp> tempPops;
	std::unordered_map<timestamp_t, size_t> tempPopsIdx;

	std::vector<QueueOp> enqs;
	std::map<val_t, QueueOp> deqs;
	std::vector<QueueOp> pendingDeqs;
	std::vector<QueueOp> pendingEnqs;

	std::vector<timestamp_t> feasibleEras;
	std::map<val_t, QueueVal> values;

	std::map<val_t, bool> hasPop;

	std::map<tid_t, event_t> active;
	ItvTree queue_tree;

	DECLHANDLER(enq);
	DECLHANDLER(deq);
	DECLHANDLER(push);
	DECLHANDLER(pop);

	void handle_crash(event_t &e);

	void simplify();

public:
	void do_linearization();
	QueueNaiveUnknownMonitor(MonitorConfig mc);
	void print_state() const;
};

#endif