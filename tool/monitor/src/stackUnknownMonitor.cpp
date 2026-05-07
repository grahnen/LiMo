#include "monitor/stackUnknownMonitor.hpp"
#include "monitor/covermonitor.h"
#include "interval.h"
#include <cassert>
#include <iostream>
#include "exception.h"
#include <climits>
#include "intervaltree.hpp"

tid_t THREAD_COUNT;

StackUnknownMonitor::StackUnknownMonitor(MonitorConfig config) : Monitor(config)
{
	// std::cout << "stack monitor created\n";
	// std :: cout << int(config.type) << "\n";
	if (!(config.type & (stack | durable_stack | unknown_after)))
	{
		throw std::logic_error("Unhandled ADT: " + ext2str(config.type));
	}
}

void StackUnknownMonitor::handle_push(event_t &e)
{
	// std::cout << e << std::endl;
	history.add_push_call(e);
}

void StackUnknownMonitor::handle_ret_push(event_t &e)
{
	// std::cout << e << std::endl;
	history.add_ret(e, false);
}

void StackUnknownMonitor::handle_pop(event_t &e)
{
	// std::cout << e << std::endl;
	history.add_pop_call(e);
}

void StackUnknownMonitor::handle_ret_pop(event_t &e)
{
	// std::cout << e << std::endl;
	history.add_ret(e, false);
}

void StackUnknownMonitor::handle_crash(event_t &e)
{

	// std::cout << e << std::endl;
	history.add_crash(e);
}

// comparator
bool comparator(const std::pair<AtomicInterval, val_t> &p1,
				const std::pair<AtomicInterval, val_t> &p2)
{
	return p1.first.lbound < p2.first.lbound;
}

// bool comparatorItv(const AtomicInterval &itv1,
// 					   const AtomicInterval &itv2)
// {
// 	return itv1.lbound < itv2.lbound;
// }

class comparatorItv
{
public:
	std::set<AtomicInterval> &empties;
	std::map<val_t, CoverVal> completed;

	comparatorItv(std::set<AtomicInterval> &emptyPop) : empties(emptyPop)
	{
	}

	comparatorItv(std::set<AtomicInterval> &emptyPop, std::map<val_t, CoverVal> &completed) : empties(emptyPop), completed(completed)
	{
	}

	constexpr bool operator()(const AtomicInterval &itv1,
							  const AtomicInterval &itv2) const
	{
		return itv1.lbound < itv2.lbound;
	}
};

// AtomicInterval getFirst(std::set<AtomicInterval> pendingPushes,
// 						std::map<val_t, CoverVal> completed,
// 						std::set<AtomicInterval> empties)
// {

// }

void InitializeSets(const std::map<val_t, CoverVal> &values,
					tsSegmentTree &segTree,
					EagerItvTree &iTree,
					std::set<val_t> &Push,
					std::set<val_t> &Pop,
					std::set<val_t> &Ext)
{
	for (auto &[v, cv] : values)
	{
		AtomicInterval LE = cv.add;
		AtomicInterval RE = cv.rmv;

		// if (verbose)
		// {
		// 	*output << "Initializing " << v << std::endl;
		// }

		bool l = false, r = false;
		if (segTree.query(LE) == 0)
		{
			l = true;
			iTree.remove(LE);
		}
		if (segTree.query(RE) == 0)
		{
			r = true;
			iTree.remove(RE);
		}

		if (l && !r)
		{
			// if(!Push.contains(v))
			Push.insert(v);
			// if (verbose)
			// {
			// 	*output << "Push\n";
			// }
		}
		else if (r && !l)
		{
			// if(!Pop.contains(v))
			Pop.insert(v);
			// if (verbose)
			// {
			// 	*output << "Pop\n";
			// }
		}
		else if (l && r)
		{
			// if(!Ext.contains(v))
			Push.erase(v);
			Pop.erase(v);
			Ext.insert(v);
			// if (verbose)
			// {
			// 	*output << "Ext\n";
			// }
		}
		else
		{
			// if (verbose)
			// {
			// 	*output << "Nothing\n";
			// }
		}
	}
}

void StackUnknownMonitor::CheckStackLin(const CoverHistory &history)
{
	try
	{
		std::map<val_t, CoverVal> values;
		for (auto &cv : history.values)
		{
			values.insert({cv.val, CoverVal(cv.val, cv.add, cv.rmv)});
		}
		if (values.empty())
		{
			return;
		}
		std::vector<timestamp_t> timestamps;
		timestamps.reserve(values.size() * 4);
		for (auto &[v, cv] : values)
		{
			timestamps.push_back(cv.add.lbound);
			timestamps.push_back(cv.add.ubound);
			timestamps.push_back(cv.rmv.lbound);
			timestamps.push_back(cv.rmv.ubound);
		}
		tsSegmentTree segTree(timestamps);
		EagerItvTree iTree;

		segTree.set_verbose(verbose);
		segTree.set_output(*output);
		iTree.set_verbose(verbose);
		iTree.set_output(output);

		std::set<val_t> Push, Pop, Ext;

		std::map<AtomicInterval, operation> itvMap;

		// Initialize Trees
		for (auto &[v, cv] : values)
		{
			AtomicInterval &LE = cv.add;
			AtomicInterval &RE = cv.rmv;

			if (verbose)
			{
				*output << v << ": " << LE << cv.cover() << RE << std::endl;
			}
			segTree.update(cv.cover(), 1);

			iTree.insert(LE);
			iTree.insert(RE);

			itvMap[LE] = operation(Epush, 0, LE.lbound, LE.ubound, v);
			itvMap[RE] = operation(Epop, 0, RE.lbound, RE.ubound, v);
		}
		compute_k(iTree.root);

		for (const auto &emptyItv : history.empties)
		{
			if (segTree.query(emptyItv) != 0)
			{
				if (iTree.root != nullptr)
				{
					iTree.deleteTree(iTree.root);
					iTree.root = nullptr;
				}
				throw Violation("Completed History not lin: empty violation");
			}
		}

		// Initialize Sets
		for (auto &[v, cv] : values)
		{
			AtomicInterval LE = cv.add;
			AtomicInterval RE = cv.rmv;

			if (verbose)
			{
				*output << "Initializing " << v << std::endl;
			}

			bool l = false, r = false;
			if (segTree.query(LE) == 0)
			{
				l = true;
				iTree.remove(LE);
			}
			if (segTree.query(RE) == 0)
			{
				r = true;
				iTree.remove(RE);
			}

			if (l && !r)
			{
				Push.insert(v);
				if (verbose)
				{
					*output << "Push\n";
				}
			}
			else if (r && !l)
			{
				Pop.insert(v);
				if (verbose)
				{
					*output << "Pop\n";
				}
			}
			else if (l && r)
			{
				Ext.insert(v);
				if (verbose)
				{
					*output << "Ext\n";
				}
			}
			else
			{
				if (verbose)
				{
					*output << "Nothing\n";
				}
			}
		}

		if (verbose)
		{
			*output << "Initialized" << std::endl;

			*output << "Push: ";
			for (auto v : Push)
			{
				*output << v << " ";
			}
			*output << std::endl;

			*output << "Pop: ";
			for (auto v : Pop)
			{
				*output << v << " ";
			}
			*output << std::endl;

			*output << "Ext: ";
			for (auto v : Ext)
			{
				*output << v << " ";
			}
			*output << std::endl;

			*output << "Segment Tree: ";
			segTree.print(*output);
		}

		while (!Ext.empty())
		{
			auto val_it = Ext.begin();
			auto v = *val_it;
			Ext.erase(val_it);

			AtomicInterval I = values.at(v).cover();
			if (verbose)
			{
				*output << "Removing " << v << ", cover: " << I << std::endl;
			}
			segTree.update(I, -1);

			long long int minVal;
			AtomicInterval minItv;
			minItv = segTree.getMinInterval(I, minVal);
			while (minVal == 0)
			{
				AtomicInterval maxOpItv;
				while (overlap(iTree.root, minItv, maxOpItv))
				{
					operation maxOp = itvMap.at(maxOpItv);
					val_t maxOpVal = maxOp.value.value();
					if (maxOp.type == Epush)
					{
						if (Pop.contains(maxOpVal))
						{
							Pop.erase(maxOpVal);
							Ext.insert(maxOpVal);
						}
						else
						{
							Push.insert(maxOpVal);
						}
					}
					else if (maxOp.type == Epop)
					{
						if (Push.contains(maxOpVal))
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
					minVal);
			}

			if (verbose)
			{
				*output << "After Removing: " << std::endl;

				*output << "Push: ";
				for (auto v : Push)
				{
					*output << v << " ";
				}
				*output << std::endl;

				*output << "Pop: ";
				for (auto v : Pop)
				{
					*output << v << " ";
				}
				*output << std::endl;

				*output << "Ext: ";
				for (auto v : Ext)
				{
					*output << v << " ";
				}
				*output << std::endl;

				*output << "Segment Tree: ";
				segTree.print(*output);
			}
		}

		if (iTree.root != nullptr)
		{
			iTree.deleteTree(iTree.root);
			iTree.root = nullptr;
			throw Violation("Completed History not Lin");
		}
		iTree.deleteTree(iTree.root);
		iTree.root = nullptr;
	}
	catch (Violation v)
	{
		throw v;
	}
	catch (Exception e)
	{
		throw Crash(e.what());
	}
	catch (std::logic_error e)
	{
		throw Crash("logic error: " + ext2str(e.what()));
	}
}

/*

timestamp_t StackUnknownMonitor::findLastValid(const std::map<val_t, CoverVal> &completedValues,
											   const std::set<AtomicInterval> &empties,
											   const AtomicInterval &push,
											   std::set<timestamp_t, std::greater<timestamp_t>> &crashedPops,
											   timestamp_t &max_ts)
{

	if (verbose)
	{
		*output << "Last Valid Called " << push << "\n";
	}
	try
	{
		const std::map<val_t, CoverVal> &values = completedValues;
		// if (values.empty())
		// {
		// 	return;
		// }
		std::vector<timestamp_t> timestamps;
		timestamps.reserve(values.size() * 4 + 2 + crashedPops.size());
		for (auto &[v, cv] : values)
		{
			timestamps.push_back(cv.add.lbound);
			timestamps.push_back(cv.add.ubound);
			timestamps.push_back(cv.rmv.lbound);
			timestamps.push_back(cv.rmv.ubound);
		}
		timestamps.push_back(push.lbound);
		timestamps.push_back(push.ubound);

		for (auto &t : crashedPops)
		{
			timestamps.push_back(t);
		}
		timestamps.push_back(++max_ts);

		tsSegmentTree segTree(timestamps);
		EagerItvTree iTree;

		segTree.set_verbose(verbose);
		segTree.set_output(*output);
		iTree.set_verbose(verbose);
		iTree.set_output(output);

		std::set<val_t> Push, Pop, Ext;

		std::map<AtomicInterval, operation> itvMap;

		auto curPopItr = crashedPops.begin();
		timestamp_t curPop = *crashedPops.begin();

		// Initialize Trees
		for (const auto &[v, cv] : values)
		{
			const AtomicInterval &LE = cv.add;
			const AtomicInterval &RE = cv.rmv;

			if (LE.overlaps(RE))
			{
				continue;
			}

			if (verbose)
			{
				*output << v << ": " << LE << cv.cover() << RE << std::endl;
			}
			segTree.update(cv.cover(), 1);

			iTree.insert(LE);
			iTree.insert(RE);

			itvMap[LE] = operation(Epush, 0, LE.lbound, LE.ubound, v);
			itvMap[RE] = operation(Epop, 0, RE.lbound, RE.ubound, v);
		}
		iTree.insert(push);
		itvMap[push] = operation(Epush, 0, push.lbound, push.ubound, -1);
		// segTree.update(push.ubound, curPop, 1);

		compute_k(iTree.root);

		// Initialize Sets
		// InitializeSets(values, segTree, iTree, Push, Pop, Ext);

		// if (segTree.query(push) == 0)
		// {
		//		return curPop;
		// }

		if (verbose)
		{
			*output << "Initialized" << std::endl;

			*output << "Push: ";
			for (auto v : Push)
			{
				*output << v << " ";
			}
			*output << std::endl;

			*output << "Pop: ";
			for (auto v : Pop)
			{
				*output << v << " ";
			}
			*output << std::endl;

			*output << "Ext: ";
			for (auto v : Ext)
			{
				*output << v << " ";
			}
			*output << std::endl;

			// *output << "Segment Tree: ";
			// segTree.print(*output);
		}

		// fix empties
		if (verbose)
			*output << "=====fixing empties=====\n";
		auto emptyItr = empties.begin();
		while (curPopItr != crashedPops.end())
		{
			curPop = *curPopItr;
			segTree.update(push.ubound, curPop, 1);
			while (emptyItr != empties.end())
			{
				if (segTree.query(*emptyItr) == 0)
				{
					emptyItr++;
				}
				else
				{
					break;
				}
			}
			if (emptyItr == empties.end())
			{
				break;
			}
			segTree.update(push.ubound, curPop, -1);
			curPopItr++;
		}
		segTree.update(push.ubound, curPop, -1);

		if (curPopItr == crashedPops.end())
		{
			if (iTree.root != nullptr)
			{
				iTree.deleteTree(iTree.root);
				iTree.root = nullptr;
			}
			return -1;
		}
		if (verbose)
		{
			*output << "=====empties fixed=====\n";
			*output << "Current Pop: " << *curPopItr << "\n";
		}
		std::map<val_t, CoverVal> remVals = values;

		while (curPopItr != crashedPops.end())
		{
			if (verbose)
			{
				*output << "Checking Pop : " << *curPopItr << "\n";
			}
			curPop = *curPopItr;
			if (curPop < push.ubound)
				break;
			segTree.update(push.ubound, curPop, 1);
			InitializeSets(remVals, segTree, iTree, Push, Pop, Ext);
			if (segTree.query(push) == 0)
			{
				// return curPop;
				break;
			}
			while (!Ext.empty())
			{
				auto val_it = Ext.begin();
				auto v = *val_it;
				Ext.erase(val_it);
				remVals.erase(v);

				AtomicInterval I = values.at(v).cover();
				if (verbose)
				{
					*output << "Removing " << v << ", cover: " << I << std::endl;
				}
				segTree.update(I, -1);

				long long int minVal;
				AtomicInterval minItv;
				minItv = segTree.getMinInterval(I, minVal);
				while (minVal == 0)
				{
					AtomicInterval maxOpItv;
					while (overlap(iTree.root, minItv, maxOpItv))
					{
						operation maxOp = itvMap.at(maxOpItv);
						val_t maxOpVal = maxOp.value.value();
						if (maxOp.type == Epush)
						{
							if (Pop.contains(maxOpVal))
							{
								Pop.erase(maxOpVal);
								Ext.insert(maxOpVal);
							}
							else
							{
								Push.insert(maxOpVal);
							}
						}
						else if (maxOp.type == Epop)
						{
							if (Push.contains(maxOpVal))
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
						minVal);
				}

				if (Push.contains(-1))
				{
					break;
				}

				if (verbose)
				{
					*output << "After Removing: " << std::endl;

					*output << "Push: ";
					for (auto v : Push)
					{
						*output << v << " ";
					}
					*output << std::endl;

					*output << "Pop: ";
					for (auto v : Pop)
					{
						*output << v << " ";
					}
					*output << std::endl;

					*output << "Ext: ";
					for (auto v : Ext)
					{
						*output << v << " ";
					}
					*output << std::endl;

					*output << "Segment Tree: ";
					segTree.print(*output);
				}
			}

			if (Push.contains(-1))
				break;

			// shift pop by one
			segTree.update(push.ubound, curPop, -1);
			curPopItr++;
			// InitializeSets(remVals, segTree, iTree, Push, Pop, Ext);
		}
		if (iTree.root != nullptr)
		{
			iTree.deleteTree(iTree.root);
			iTree.root = nullptr;
		}

		if (curPopItr == crashedPops.end())
		{
			return -1;
		}
		else
		{
			if (verbose)
			{
				*output << "LAstValid returning " << curPop << "\n";
			}
			return curPop;
		}

		// if (iTree.root != nullptr)
		// {
		// 	iTree.deleteTree(iTree.root);
		// 	iTree.root = nullptr;
		// 	throw Violation("Ext empty, h not empty");
		// }
		// iTree.root = nullptr;
	}
	catch (Violation v)
	{
		throw v;
	}
	catch (Exception e)
	{
		throw Crash(e.what());
	}
	catch (std::logic_error e)
	{
		throw Crash("logic error: " + ext2str(e.what()));
	}
}

timestamp_t StackUnknownMonitor::findFirstValid(const std::map<val_t, CoverVal> &completedValues,
												const std::set<AtomicInterval> &empties,
												const AtomicInterval &push,
												std::set<timestamp_t> &crashedPops,
												timestamp_t &max_ts,
												std::set<timestamp_t>::iterator begin)
{

	if (verbose)
	{
		*output << "First Valid Called " << push << "\n";
	}
	try
	{
		const std::map<val_t, CoverVal> &values = completedValues;
		// if (values.empty())
		// {
		// 	return;
		// }
		std::vector<timestamp_t> timestamps;
		timestamps.reserve(values.size() * 4 + 2 + crashedPops.size());
		for (auto &[v, cv] : values)
		{
			timestamps.push_back(cv.add.lbound);
			timestamps.push_back(cv.add.ubound);
			timestamps.push_back(cv.rmv.lbound);
			timestamps.push_back(cv.rmv.ubound);
		}
		timestamps.push_back(push.lbound);
		timestamps.push_back(push.ubound);

		for (auto &t : crashedPops)
		{
			timestamps.push_back(t);
		}
		timestamps.push_back(++max_ts);

		tsSegmentTree segTree(timestamps);
		EagerItvTree iTree;

		segTree.set_verbose(verbose);
		segTree.set_output(*output);
		iTree.set_verbose(verbose);
		iTree.set_output(output);

		std::set<val_t> Push, Pop, Ext;

		std::map<AtomicInterval, operation> itvMap;

		if (begin == crashedPops.end())
			return -1;
		auto curPopItr = begin;
		timestamp_t curPop = *curPopItr;

		// Initialize Trees
		for (const auto &[v, cv] : values)
		{
			const AtomicInterval &LE = cv.add;
			const AtomicInterval &RE = cv.rmv;

			if (LE.overlaps(RE))
			{
				continue;
			}

			if (verbose)
			{
				*output << v << ": " << LE << cv.cover() << RE << std::endl;
			}
			segTree.update(cv.cover(), 1);

			iTree.insert(LE);
			iTree.insert(RE);

			itvMap[LE] = operation(Epush, 0, LE.lbound, LE.ubound, v);
			itvMap[RE] = operation(Epop, 0, RE.lbound, RE.ubound, v);
		}
		iTree.insert(push);
		itvMap[push] = operation(Epush, 0, push.lbound, push.ubound, -1);
		// segTree.update(push.ubound, curPop, 1);

		compute_k(iTree.root);

		// Initialize Sets
		// InitializeSets(values, segTree, iTree, Push, Pop, Ext);

		// if (segTree.query(push) == 0)
		// {
		//		return curPop;
		// }

		if (verbose)
		{
			*output << "Initialized" << std::endl;

			*output << "Push: ";
			for (auto v : Push)
			{
				*output << v << " ";
			}
			*output << std::endl;

			*output << "Pop: ";
			for (auto v : Pop)
			{
				*output << v << " ";
			}
			*output << std::endl;

			*output << "Ext: ";
			for (auto v : Ext)
			{
				*output << v << " ";
			}
			*output << std::endl;

			// *output << "Segment Tree: ";
			// segTree.print(*output);
		}

		// fix empties
		if (verbose)
			*output << "=====fixing empties=====\n";
		auto emptyItr = empties.begin();
		while (curPopItr != crashedPops.end())
		{
			curPop = *curPopItr;
			segTree.update(push.ubound, curPop, 1);
			while (emptyItr != empties.end())
			{
				if (segTree.query(*emptyItr) == 0)
				{
					emptyItr++;
				}
				else
				{
					break;
				}
			}
			if (emptyItr == empties.end())
			{
				break;
			}
			segTree.update(push.ubound, curPop, -1);
			curPopItr++;
		}
		segTree.update(push.ubound, curPop, -1);

		if (curPopItr == crashedPops.end())
		{
			if (iTree.root != nullptr)
			{
				iTree.deleteTree(iTree.root);
				iTree.root = nullptr;
			}
			return -1;
		}
		if (verbose)
		{
			*output << "=====empties fixed=====\n";
			*output << "Current Pop: " << *curPopItr << "\n";
		}
		std::map<val_t, CoverVal> remVals = values;

		while (curPopItr != crashedPops.end())
		{
			if (verbose)
			{
				*output << "Checking Pop : " << *curPopItr << "\n";
			}
			curPop = *curPopItr;
			if (curPop < push.ubound)
				break;
			segTree.update(push.ubound, curPop, 1);
			InitializeSets(remVals, segTree, iTree, Push, Pop, Ext);
			if (segTree.query(push) == 0)
			{
				// return curPop;
				break;
			}
			while (!Ext.empty())
			{
				auto val_it = Ext.begin();
				auto v = *val_it;
				Ext.erase(val_it);
				remVals.erase(v);

				AtomicInterval I = values.at(v).cover();
				if (verbose)
				{
					*output << "Removing " << v << ", cover: " << I << std::endl;
				}
				segTree.update(I, -1);

				long long int minVal;
				AtomicInterval minItv;
				minItv = segTree.getMinInterval(I, minVal);
				while (minVal == 0)
				{
					AtomicInterval maxOpItv;
					while (overlap(iTree.root, minItv, maxOpItv))
					{
						operation maxOp = itvMap.at(maxOpItv);
						val_t maxOpVal = maxOp.value.value();
						if (maxOp.type == Epush)
						{
							if (Pop.contains(maxOpVal))
							{
								Pop.erase(maxOpVal);
								Ext.insert(maxOpVal);
							}
							else
							{
								Push.insert(maxOpVal);
							}
						}
						else if (maxOp.type == Epop)
						{
							if (Push.contains(maxOpVal))
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
						minVal);
				}

				if (Push.contains(-1))
				{
					break;
				}

				if (verbose)
				{
					*output << "After Removing: " << std::endl;

					*output << "Push: ";
					for (auto v : Push)
					{
						*output << v << " ";
					}
					*output << std::endl;

					*output << "Pop: ";
					for (auto v : Pop)
					{
						*output << v << " ";
					}
					*output << std::endl;

					*output << "Ext: ";
					for (auto v : Ext)
					{
						*output << v << " ";
					}
					*output << std::endl;

					*output << "Segment Tree: ";
					segTree.print(*output);
				}
			}

			if (Push.contains(-1))
				break;

			// shift pop by one
			segTree.update(push.ubound, curPop, -1);
			curPopItr++;
			// InitializeSets(remVals, segTree, iTree, Push, Pop, Ext);
		}
		if (iTree.root != nullptr)
		{
			iTree.deleteTree(iTree.root);
			iTree.root = nullptr;
		}

		if (curPopItr == crashedPops.end())
		{
			return -1;
		}
		else
		{
			if (verbose)
			{
				*output << "LastValid returning " << curPop << "\n";
			}
			return curPop;
		}

		// if (iTree.root != nullptr)
		// {
		// 	iTree.deleteTree(iTree.root);
		// 	iTree.root = nullptr;
		// 	throw Violation("Ext empty, h not empty");
		// }
		// iTree.root = nullptr;
	}
	catch (Violation v)
	{
		throw v;
	}
	catch (Exception e)
	{
		throw Crash(e.what());
	}
	catch (std::logic_error e)
	{
		throw Crash("logic error: " + ext2str(e.what()));
	}
	catch(...)
	{
		throw Crash("Fault");
	}
}
*/
/*
// this is n^2 log n implementation
// void StackUnknownMonitor::do_linearization()
// {
// 	// verbose = false;
// 	try
// 	{
// 		if (!history.complete())
// 			throw Violation("incomplete history");
// 		history.simplify();
// 		// Find Maximum Timestamp
// 		timestamp_t max_ts = 0;
// 		std::set<timestamp_t> timestampsSet;
// 		std::set<timestamp_t> crashedPopsSorted;
// 		CoverHistory complHist;
// 		for (auto &[v, cv] : history.completedValues)
// 		{
// 			if (cv.add.ubound >= cv.rmv.lbound)
// 				history.completedValues.erase(v);
// 			max_ts = max(max_ts, cv.rmv.ubound);
// 			timestampsSet.insert(cv.add.lbound);
// 			timestampsSet.insert(cv.add.ubound);
// 			timestampsSet.insert(cv.rmv.lbound);
// 			timestampsSet.insert(cv.rmv.ubound);
// 			complHist.values.insert(cv);
// 		}

// 		for (auto &[v, itv] : history.pendingPush)
// 		{
// 			max_ts = max(max_ts, itv.ubound);
// 			timestampsSet.insert(itv.lbound);
// 			timestampsSet.insert(itv.ubound);
// 			// std::cout << "Push " << itv << "\n";
// 		}

// 		for (auto &ts : history.crashedPops)
// 		{
// 			max_ts = max(max_ts, ts);
// 			timestampsSet.insert(ts);
// 			crashedPopsSorted.insert(ts);
// 			// std::cout << "Pop " << ts << "\n";
// 			// std::cout<<"inserting " << ts << std::endl;
// 		}

// 		for (auto &itv : history.emptyPop)
// 		{
// 			max_ts = max(max_ts, itv.ubound);
// 			timestampsSet.insert(itv.lbound);
// 			timestampsSet.insert(itv.ubound);
// 			complHist.empties.push_back(itv);
// 		}

// 		// cout << "Max ts: " << max_ts << "\n";
// 		// We have to complete the history too
// 		// Add the req. number of crashed pops at the end
// 		if (history.pendingPush.size() > history.crashedPops.size())
// 		{
// 			while (history.crashedPops.size() < history.pendingPush.size())
// 			{
// 				history.crashedPops.insert(++max_ts);
// 				timestampsSet.insert(max_ts);
// 			}
// 		}

// 		timestampsSet.insert(++max_ts);
// 		std::vector<timestamp_t> timestampsVec(timestampsSet.begin(), timestampsSet.end());

// 		// std::cout<<"Timestamps: ";
// 		// for(auto ts : timestampsVec)
// 		// {
// 		// 	std::cout << ts << " ";
// 		// }
// 		// std::cout<< "\n";

// 		// std::cout << "Max-ts" << max_ts << "\n";

// 		// Check comleted sub-history linearizability
// 		CheckStackLin(complHist);

// 		if (verbose)
// 		{
// 			*output << "Completed Subhistory Linearizable================\n";
// 		}
// 		// Check Feasibility of all pops
// 		// maintain a segment tree of #(push-ret<=ts) - #(pop<=ts)

// 		tsSegmentTree feasibleTree(timestampsVec);
// 		timestamp_t max_tsSegTree = timestampsVec.back();

// 		feasibleTree.set_verbose(verbose);
// 		feasibleTree.set_output(*output);

// 		std::map<AtomicInterval, val_t> pendingPushVal;
// 		// comparatorItv pushOrdering(history.emptyPop);
// 		std::set<AtomicInterval, comparatorItv> pendingPushSorted(comparatorItv(history.emptyPop));

// 		for (auto &[v, itv] : history.pendingPush)
// 		{
// 			feasibleTree.update(itv.ubound, max_tsSegTree, 1);

// 			pendingPushVal.emplace(itv, v);
// 			pendingPushSorted.insert(itv);
// 		}
// 		for (auto &ts : history.crashedPops)
// 		{
// 			feasibleTree.update(ts, max_tsSegTree, -1);
// 		}

// 		// For pushes in order:
// 		//	Find last pop
// 		//	Find last feasible before it
// 		//  	assign
// 		//  	remove push,pop and add to completed
// 		//	update feasible

// 		for (auto &itv : pendingPushSorted)
// 		{
// 			if (verbose)
// 			{
// 				*output << "Checking push " << itv << "============\n";
// 			}
// 			timestamp_t lastValid = findFirstValid(history.completedValues,
// 												   history.emptyPop,
// 												   itv, crashedPopsSorted,
// 												   max_ts, crashedPopsSorted.begin());
// 			if (verbose)
// 			{
// 				*output << "First Valid returned " << lastValid << "\n";
// 			}
// 			if (lastValid == -1)
// 			{
// 				throw Violation("no valid pop");
// 			}
// 			auto popItr = crashedPopsSorted.find(lastValid);
// 			timestamp_t lastFeasible = lastValid;
// 			// cout<<*popItr<< " " << lastFeasible<<"\n";
// 			while (feasibleTree.elementRight(lastFeasible) > 0)
// 			{
// 				// cout<<*popItr<< " " << lastFeasible<<"\n";
// 				std::cout << "not-feasible\n";
// 				popItr++;
// 				if (popItr == crashedPopsSorted.end())
// 				{
// 					throw Violation("no feasible pop found");
// 				}
// 				lastFeasible = *popItr;
// 				lastValid = findFirstValid(history.completedValues,
// 										   history.emptyPop,
// 										   itv, crashedPopsSorted,
// 										   max_ts,
// 										   popItr);
// 				if(verbose)	std::cout << "First Valid returned: " << lastValid << "\n";
// 				popItr = crashedPopsSorted.find(lastValid);
// 				lastFeasible = lastValid;
// 				if(popItr == crashedPopsSorted.end())
// 					throw Violation("no feasible pop found");
// 			}
// 			if (verbose)
// 				cout << "Pop found: " << lastFeasible << "\n";
// 			// cout<<itv.ubound<<" "<<max_tsSegTree << " "<<lastFeasible << std::endl;
// 			history.completedValues.emplace(pendingPushVal[itv], CoverVal(pendingPushVal[itv], itv, AtomicInterval(true, lastFeasible, max_ts, true)));
// 			// history.crashedPops.erase(popItr);
// 			crashedPopsSorted.erase(popItr);

// 			// cout<<itv.ubound<<" "<<max_tsSegTree << " "<<lastFeasible << std::endl;
// 			feasibleTree.update(itv.ubound, max_tsSegTree, -1);
// 			feasibleTree.update(lastFeasible, max_tsSegTree, +1);
// 		}

// 		return;
// 	}
// 	catch (Violation v)
// 	{
// 		throw v;
// 	}
// 	catch (Crash c)
// 	{
// 		throw c;
// 	}
// 	catch (Exception e)
// 	{
// 		throw Crash(string("Exception: ") + e.what());
// 	}
// 	catch (std::logic_error r)
// 	{
// 		throw Crash(string("logic_error") + r.what());
// 	}
// 	catch (...)
// 	{
// 		throw Crash("fault");
// 		// throw std::bad_exception();
// 	}
// }

*/

tsSegmentTree StackUnknownMonitor::computeCounters()
{
	timestamp_t unm_push = 0;
	timestamp_t unm_pop = 0;
	timestamp_t ctr = 0;
	timestamp_t era = 0;

	auto counters = tsSegmentTree(history.times);

	auto& pushes = history.push_ret;
	auto& pops   = history.pop_call;
	auto& crashes= history.crashes;

	auto pushIt = pushes.begin();
	auto popIt  = pops.begin();
	auto crashIt = crashes.begin();

	while(pushIt != pushes.end() || popIt != pops.end() || crashIt != crashes.end())
	{
		timestamp_t pushTime = (pushIt != pushes.end() ? *pushIt : POSINF);
		timestamp_t popTime  = (popIt != pops.end() ? *popIt : POSINF);
		timestamp_t crashTime= (crashIt != crashes.end() ? *crashIt : POSINF);

		timestamp_t time = std::min(pushTime, std::min(popTime, crashTime));

		int del_ctr = 0;
		auto& evt = history.events[time];
		if(time == pushTime)
		{
			auto val = evt.val.value();
			bool isUnm = history.pendingPush.contains(val);
			if(unm_pop > 0 && isUnm)
			{
				unm_pop --;
			}
			else
			{
				ctr++;
				del_ctr = 1;
				if(isUnm)
					unm_push++;
			}
			pushIt++;
		}
		else if(time == popTime && history.crashedPops.contains(time))
		{
			if(unm_push > 0)
			{
				ctr--;
				del_ctr = -1;
				unm_push--;
			}
			else
			{
				unm_pop > 0;
			}
			popIt++;
		}
		else if(time == popTime)
		{
			ctr--;
			del_ctr = -1;
			popIt++;
		}
		else if(time == crashTime)
		{
			unm_pop = 0;
			era++;
			crashIt++;
		}

		counters.update(time, history.times.back(), del_ctr);
	}

	return counters;
}

void StackUnknownMonitor::do_linearization()
{
	history.simplify();
	auto counters = computeCounters();
	counters.print();
}