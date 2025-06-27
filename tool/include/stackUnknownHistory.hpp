#ifndef STACK_UNKNOWN_HISTORY_H
#define STACK_UNKNOWN_HISTORY_H

#include "history.h"
#include "interval.h"

struct DCoverVal{
	val_t val;
	AtomicInterval add;
	AtomicInterval rmv;
};

class StackUnknownHistoryAfter : public History<StackUnknownHistoryAfter>
{
public:
	using LinRes = History<StackUnknownHistoryAfter>::LinRes;

	std::map<val_t, DCoverVal> completedValues;
	
	std::map<val_t, AtomicInterval> pendingPush;
	std::map<val_t, event_t> crashedPush;
	std::set<timestamp_t, std::greater<timestamp_t>> crashedPops;
	std::set<AtomicInterval> emptyPop;


	std::map<tid_t, event_t> activeOperations;

	void add_push_call(event_t& call);
	void add_pop_call(event_t& call);
	void add_crash(event_t& crash);
	LinRes add_ret(event_t& ret, bool crash);

	LinRes step();
	bool complete() const;
	std::map<timestamp_t, event_t> createTimestampedHistory(tid_t&, timestamp_t&);

	void print_state() const
	{
		std::cout <<"Completed values: \n";
		for(auto&[val,cv] : completedValues)
			std::cout<<val<<" "<<cv.add<<" "<<cv.rmv<<"\n";
		std::cout<<"\nPending Push: \n";
		for(auto&[val,I] : pendingPush)
			std::cout<<val<<" "<<I<<"\n";
		std::cout<<"\nEmptyPops: \n";
		for(auto&I : emptyPop)
			std::cout<<I<<"\n";
		std::cout<<"\n\n";
	}
};

#endif //STACK_UNKNOWN_HISTORY_H