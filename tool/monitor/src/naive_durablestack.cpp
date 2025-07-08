#include "monitor/naive_durable_stack.hpp"
#include <cassert>
#include <iostream>
#include "exception.h"

NaiveDurableStackMonitor::NaiveDurableStackMonitor(MonitorConfig mc) : Monitor(mc)
{
	if (!(mc.type & (durable_stack | stack)))
	{
		throw std::logic_error("Unhandled ADT: " + ext2str(mc.type));
	}
}

void NaiveDurableStackMonitor::handle_push(event_t &e)
{
	// history.add_push_call(e);
}

void NaiveDurableStackMonitor::handle_ret_push(event_t &e)
{
	// history.add_ret(e);
}

void NaiveDurableStackMonitor::handle_pop(event_t &e)
{
	// history.add_pop_call(e);
}

void NaiveDurableStackMonitor::handle_ret_pop(event_t &e)
{
	// history.add_ret(e);
}

void NaiveDurableStackMonitor::print_state() const
{
	*output << history << std::endl;
}

void NaiveDurableStackMonitor::handle_crash(event_t &e)
{
	std::cout << "Crash!" << std::endl;
}

void NaiveDurableStackMonitor::do_linearization()
{
	// if (!history.sound()) {
	//   throw Violation("Unmatched Pop");
	// }
	// if (!history.simple()) {
	//   throw std::runtime_error("Simplified history is still not simple.");
	// }
	// if(verbose)
	//   *output << "Linearizing: " << history << std::endl;
	// LinRes lr = history.step();
	// while (!lr.violation() && lr.remaining.size() > 0) {
	//   auto oa = lr.remaining.back();
	//   lr.remaining.pop_back();
	//   if (verbose) {
	//     *output << "Linearizing: " << oa << std::endl;
	//   }
	//   LinRes lr2 = oa.step();
	//   lr = lr + lr2;
	// }
	// if (lr.violation()) {
	//   throw Violation(lr.error());
	// }
}

// Class NaiveDStackUnknown
NaiveDStackUnknown::NaiveDStackUnknown(MonitorConfig config) : StackUnknownMonitor(config)
{
	if (!(config.type & (stack | durable_stack | unknown_after)) )
	{
		throw std::logic_error("Unhandled ADT: " + ext2str(config.type));
	}
}

void NaiveDStackUnknown::do_linearization()
{
	using std::max;

	history.simplify();

	std::vector<val_t> pushVals;
	std::set<CoverVal> values;
	pushVals.reserve(history.pendingPush.size());

	if (!history.complete())
		throw Violation("incomplete history");
	// Find Maximum Timestamp
	timestamp_t max_ts = 0;
	std::set<timestamp_t> timestampsSet;
	for (auto &[v, cv] : history.completedValues)
	{
		if (cv.add.ubound >= cv.rmv.lbound)
			history.completedValues.erase(v);
		max_ts = max(max_ts, cv.rmv.ubound);
		timestampsSet.insert(cv.add.lbound);
		timestampsSet.insert(cv.add.ubound);
		timestampsSet.insert(cv.rmv.lbound);
		timestampsSet.insert(cv.rmv.ubound);

		values.emplace(v, cv.add, cv.rmv);
	}

	for (auto &[v, itv] : history.pendingPush)
	{
		max_ts = max(max_ts, itv.ubound);
		timestampsSet.insert(itv.lbound);
		timestampsSet.insert(itv.ubound);

		pushVals.push_back(v);
	}

	for (auto &ts : history.crashedPops)
	{
		max_ts = max(max_ts, ts);
		timestampsSet.insert(ts);
	}

	for (auto &itv : history.emptyPop)
	{
		max_ts = max(max_ts, itv.ubound);
		timestampsSet.insert(itv.lbound);
		timestampsSet.insert(itv.ubound);
	}

	// We have to complete the history too
	// Add the req. number of crashed pops at the end
	if (history.pendingPush.size() > history.crashedPops.size())
	{
		while (history.pendingPush.size() >  history.crashedPops.size())
		{
			history.crashedPops.insert(++max_ts);
			timestampsSet.insert(max_ts);
		}
	}

	std::vector<timestamp_t> timestampsVec(timestampsSet.begin(), timestampsSet.end());

	sort(pushVals.begin(), pushVals.end());
	
	std::vector<AtomicInterval> crashedPopItv;
	crashedPopItv.reserve(history.crashedPops.size());

	for(auto ts : history.crashedPops)
	{
		crashedPopItv.emplace_back(true, ts, ++max_ts, true);
		timestampsSet.insert(max_ts);
	}

	std::vector<AtomicInterval> empties(history.emptyPop.begin(), history.emptyPop.end());

	bool foundLin = false;

	do{
		CoverHistory hist;
		hist.empties = std::move(empties);
		hist.values = values;

		if(verbose)
		{
			*output << "=====Trying new Permutation=====\n";
			for(auto i : pushVals)
				*output<<i<<" ";
			*output << "\n";
		}

		auto popItr = crashedPopItv.rbegin();
		for(auto v : pushVals)
		{
			if(popItr == crashedPopItv.rend())
			{
				throw Violation("Not enough pops");
			}
			if(history.pendingPush.at(v).ubound >= popItr->lbound)
			{
				popItr++;
				continue;
			}
			hist.values.emplace(v, history.pendingPush.at(v), *popItr);
			popItr++;	
		}

		bool caughtViolation = false;
		try
		{
			CheckStackLin(hist);
		}
		catch(Violation v)
		{
			caughtViolation = true;
			if(verbose)
				*output << "=====Unlin=====\n";
		}
		
		if(!caughtViolation)
		{
			foundLin = true;
			if(verbose)
				*output << "======Lin======\n";
			break;
		}

		empties = std::move(hist.empties);
	}while(std::next_permutation(pushVals.begin(), pushVals.end()));

	if(!foundLin)
		throw Violation("no permutaion is linearizable");

}
