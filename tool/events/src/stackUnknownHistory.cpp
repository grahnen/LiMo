#include "stackUnknownHistory.hpp"
#include <ostream>
#include <iostream>
#include "exception.h"
#include "interval.h"
#include <algorithm>

void StackUnknownHistoryAfter::add_push_call(event_t& call)
{
	if(! call.val)
		throw Violation("push called without value " + ext2str(call));
	activeOperations[call.thread] = call;
}

void StackUnknownHistoryAfter::add_pop_call(event_t& call)
{
	activeOperations[call.thread] = call;
}


using LinRes = History<StackUnknownHistoryAfter>::LinRes;
LinRes StackUnknownHistoryAfter::add_ret(event_t& ret, bool crash = false)
{
	// std::cout << "add_ret called - ";
	if(!activeOperations.contains(ret.thread))
	{
		throw Violation("return before call " + ext2str(ret));
	}

	event_t call = std::move(activeOperations[ret.thread]);
	activeOperations.erase(ret.thread);
	if(call.type == Epush)
	{
		pendingPush[call.val.value()] = AtomicInterval(false, call.timestamp, ret.timestamp, false);
		// std::cout << "push-ret-" << pendingPush[call.val.value()] << std::endl;
	}
	else if(call.type == Epop)
	{
		if(!ret.val)
		{
			// std::cout << "empty-pop\n";
			//throw Exception("unhandled - empty pop");
			AtomicInterval popInt(false, call.timestamp, ret.timestamp, false);
			emptyPop.insert(popInt);
			return LinRes();
		}
		if(!pendingPush.contains(ret.val.value()))
		{
			if(!crashedPush.contains(ret.val.value()))
			{
				throw Violation("pop without push" + ext2str(ret));
			}
			else
			{
				// std::cout << "crashed push-pop\n";
				crashedPush.erase(ret.val.value());
				return LinRes();
			}
		}
		AtomicInterval popInt(false, call.timestamp, ret.timestamp, false);
		AtomicInterval pushInt = std::move(pendingPush[ret.val.value()]);
		pendingPush.erase(ret.val.value());
		completedValues.emplace(ret.val.value(), CoverVal(ret.val.value(), pushInt, popInt));
		// std::cout << "Comleted Value: " << ret.val.value() << ", " << pushInt << popInt << std::endl; 
	}

	return LinRes();
}

void StackUnknownHistoryAfter::add_crash(event_t& crash)
{
	for(auto [tid, event] : activeOperations)
	{
		if(event.type == Epush)
		{
			crashedPush[event.val.value()] =  event;
		}
		else if(event.type == Epop)
		{
			crashedPops.insert(event.timestamp);
		}
	}

	activeOperations.clear();
}

bool StackUnknownHistoryAfter::complete() const
{
	return activeOperations.size() == 0;
}

void StackUnknownHistoryAfter::simplify()
{
	if(!complete())
	{
		throw Exception("siomplifying incomplete history");
	}

	for(auto &[v, cv] : completedValues)
	{
		if(cv.add.lbound > cv.rmv.ubound)
			throw Violation("pop before push");

		if(cv.add.ubound > cv.rmv.lbound)
			completedValues.erase(v);
	}	


}

StackUnknownHistoryAfter::LinRes StackUnknownHistoryAfter::step() 
{
	return LinRes();
}

std::map<timestamp_t, event_t> StackUnknownHistoryAfter::createTimestampedHistory(tid_t& numThreads, timestamp_t& max_ts)
{
	std::map<timestamp_t, event_t> tsHistory;
	
	for(auto &[val, cv] : completedValues)
	{
		AtomicInterval& push = cv.add;
		AtomicInterval& pop  = cv.rmv;
		
		tsHistory[push.lbound] = event_t(Epush, numThreads, val, push.lbound);
		tsHistory[push.ubound] = event_t(Ereturn, numThreads++, val, push.ubound);
		tsHistory[pop.lbound] = event_t(Epop, numThreads, val, pop.lbound);
		tsHistory[pop.ubound] = event_t(Ereturn, numThreads++, val, pop.ubound);

		max_ts = std::max(max_ts, push.lbound);
		max_ts = std::max(max_ts, push.ubound);
		max_ts = std::max(max_ts, pop.lbound);
		max_ts = std::max(max_ts, pop.ubound);
	}
	
	for(auto &I : emptyPop)
	{
		tsHistory[I.lbound] = event_t(Epop, numThreads, {}, I.lbound);
		tsHistory[I.ubound] = event_t(Ereturn, numThreads++, {}, I.ubound);
		max_ts = std::max(max_ts, I.lbound);
		max_ts = std::max(max_ts, I.ubound);

	}

	return tsHistory;
}