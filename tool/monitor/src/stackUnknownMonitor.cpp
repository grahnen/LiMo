#include "monitor/stackUnknownMonitor.hpp"
#include "monitor/covermonitor.h"
#include "interval.h"
#include <cassert>
#include <iostream>
#include "exception.h"
#include <climits>
#include "intervaltree.hpp"
#include "segment_tree.hpp"

tid_t THREAD_COUNT;

StackUnknownMonitor::StackUnknownMonitor(MonitorConfig config) : Monitor(config)
{
	// std::cout << "stack monitor created\n"; 
	if (!config.type & stack)
	{
		throw std::logic_error("Unhandled ADT: " + ext2str(config.type));
	}
}

void StackUnknownMonitor::handle_push(event_t &e)
{
	std::cout << e << std::endl;
	history.add_push_call(e);
}

void StackUnknownMonitor::handle_ret_push(event_t &e)
{
	std::cout << e << std::endl;
	history.add_ret(e, false);
}

void StackUnknownMonitor::handle_pop(event_t &e)
{
	std::cout << e << std::endl;
	history.add_pop_call(e);
}

void StackUnknownMonitor::handle_ret_pop(event_t &e)
{
	std::cout << e << std::endl;
	history.add_ret(e, false);
}

void StackUnknownMonitor::handle_crash(event_t &e)
{
	
	std::cout << e << std::endl;
	history.add_crash(e);
}

// comparator
static bool comparator(const std::pair<AtomicInterval, val_t> &p1,
					   const std::pair<AtomicInterval, val_t> &p2)
{
	return p1.first.lbound < p2.first.lbound;
}

static bool CheckCoverLin(std::map<val_t, DCoverVal> values)
{
	std::vector<timestamp_t> timestamps;
	timestamps.reserve(values.size() * 4);
	for(auto& [v, cv] : values)
	{
		timestamps.push_back(cv.add.lbound);
		timestamps.push_back(cv.add.ubound);
		timestamps.push_back(cv.rmv.lbound);
		timestamps.push_back(cv.rmv.ubound);

	}
	tsSegmentTree segTree(timestamps);
	EagerItvTree iTree;

	std::set<val_t> Push, Pop, Ext;

	std::map<AtomicInterval, operation> itvMap;
	
	//Initialize Trees
	for(auto & [v, cv] : values)
	{
		AtomicInterval LE = AtomicInterval(true,cv.add.lbound,cv.add.ubound,true);
		AtomicInterval RE = AtomicInterval(true,cv.rmv.lbound,cv.rmv.ubound,true);
		
		segTree.update(cv.add.ubound,cv.rmv.lbound, 1);

		iTree.insert(LE);
		iTree.insert(RE);

		itvMap[LE] = operation(Epush, 0, LE.lbound, LE.ubound, v);
		itvMap[RE] = operation(Epop, 0, RE.lbound, RE.ubound, v);
	}
	compute_k(iTree.root);

	//Initialize Sets
	for(auto & [v, cv] : values)
	{
		AtomicInterval LE = AtomicInterval(true,cv.add.lbound,cv.add.ubound,true);
		AtomicInterval RE = AtomicInterval(true,cv.rmv.lbound,cv.rmv.ubound,true);
		
		bool l=false, r=false;
		if(segTree.query(LE) == 0)
		{
			l = true;
			iTree.remove(LE);
			
		}
		if(segTree.query(RE) == 0)
		{
			r = true;
			iTree.remove(RE);
		}

		if(l && ~r)
		{
			Push.insert(v);
		}
		else if(r && !l)
		{
			Pop.insert(v);
		}
		else if(l && r)
		{
			Ext.insert(v);
		}
	}

	while(!Ext.empty())
	{
		auto val_it = Ext.begin();
		auto v = *val_it;
		Ext.erase(val_it);
		AtomicInterval I = AtomicInterval(true, values[v].add.ubound, values[v].rmv.lbound, true);
		segTree.update(I, -1);

		long long int minVal;
		AtomicInterval minItv;
		minItv = segTree.getMinInterval(I, minVal);
		while(minVal == 0)
		{
			AtomicInterval maxOpItv;
			while(overlap(iTree.root, minItv, maxOpItv))
			{
				operation maxOp = itvMap[maxOpItv];
				val_t maxOpVal = maxOp.value.value();
				if(maxOp.type == Epush)
				{
					if(Pop.contains(maxOpVal))
					{
						Pop.erase(maxOpVal);
						Ext.insert(maxOpVal);
					}
					else
					{
						Push.insert(maxOpVal);
					}
				}
				else if(maxOp.type == Epop)
				{
					if(Push.contains(maxOpVal))
					{
						Push.erase(maxOpVal);
						Ext.insert(maxOpVal);
					}
					else
					{
						Pop.insert(maxOpVal);
					}
				}

				iTree.remove(maxOpItv);
			}

			minItv = segTree.getMinInterval(
						AtomicInterval(true, minItv.ubound, I.ubound, true),
						minVal
					);
		}
	}

	if(iTree.root!=nullptr)
		return false;

	return true;	

}

// this is n^3 log n implementation
void StackUnknownMonitor::do_linearization()
{

	if(verbose)
		print_state();

	throw Crash("unimplemented");

	#ifdef IMPL_STACK_UNKNOWN

	std::vector<std::pair<AtomicInterval, val_t>> pendingPushSorted;
	pendingPushSorted.reserve(history.pendingPush.size());
	for (auto [v, I] : history.pendingPush)
		pendingPushSorted.push_back({I, v});

	std::sort(pendingPushSorted.begin(), pendingPushSorted.end(), comparator);

	THREAD_COUNT = 0;
	timestamp_t TIMESTAMP;
	std::map<timestamp_t, event_t> tsCompletedHistory = history.createTimestampedHistory(THREAD_COUNT, TIMESTAMP);

	// if(!checkCoverLin(tsCompletedHistory))
	{
		std::cout << "Throwing violation\n";
		throw Violation("Completed History unlinearizable");
	}
	// else
	// {
	// 	std::cout<<"Nooooooooo\n";
	// }
	for(auto &[I, v] : pendingPushSorted)
	{
		tsCompletedHistory[I.lbound] = event_t(Epush, THREAD_COUNT, v, I.lbound);
		tsCompletedHistory[I.ubound] = event_t(Ereturn, THREAD_COUNT, v, I.ubound);
		bool done = false;
		for(auto pop : history.crashedPops)
		{
			tsCompletedHistory[pop] 	 = event_t(Epop, THREAD_COUNT+1, v, pop);
			tsCompletedHistory[TIMESTAMP]= event_t(Ereturn, THREAD_COUNT+1, v, TIMESTAMP);


			// if(checkCoverLin(tsCompletedHistory))
			{
				done = true;
				THREAD_COUNT+=2;
				history.crashedPops.erase(pop);
				TIMESTAMP++;
				break;
			}
			// else
			{
				tsCompletedHistory.erase(pop);
				tsCompletedHistory.erase(TIMESTAMP);
			}
		}
		if(!done)
		{
			std::cout << "Throwing Violation\n";
			throw Violation("No Valid pop left");
		}

	}
	// if(!checkCoverLin(tsCompletedHistory))
	{
		std::cout << "throwing violation\n";
		throw Violation("Completed History unlinearizable");
	}

	#endif
}