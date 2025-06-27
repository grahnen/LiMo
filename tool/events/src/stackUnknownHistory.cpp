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
	if(!activeOperations.contains(ret.thread))
	{
		throw Violation("return before call " + ext2str(ret));
	}

	event_t call = std::move(activeOperations[ret.thread]);
	activeOperations.erase(ret.thread);
	if(call.type == Epush)
	{
		pendingPush[call.val.value()] = AtomicInterval(false, call.timestamp, ret.timestamp, false);
	}
	else if(call.type == Epop)
	{
		if(!ret.val)
		{
			//throw Exception("unhandles - empty pop");
			AtomicInterval popInt(false, call.timestamp, ret.timestamp, false);
			emptyPop.insert(popInt);
			return LinRes();
		}
		if(pendingPush.find(ret.val.value()) == pendingPush.end())
		{
			if(crashedPush.find(ret.val.value()) == crashedPush.end())
			{
				throw Violation("pop without push" + ext2str(ret));
			}
			else
			{
				crashedPush.erase(ret.val.value());
				return LinRes();
			}
		}
		AtomicInterval popInt(false, call.timestamp, ret.timestamp, false);
		AtomicInterval pushInt = std::move(pendingPush[ret.val.value()]);
		pendingPush.erase(ret.val.value());
		completedValues[ret.val.value()] = {ret.val.value(), pushInt, popInt};
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