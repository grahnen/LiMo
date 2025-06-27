#include "monitor/stackoptimalmonitor.hpp"
#include "interval.h"
#include <cassert>
#include <iostream>
#include "exception.h"
#include "intervaltree.hpp"
#include "segment_tree.hpp"

StackOptimalMonitor::StackOptimalMonitor(MonitorConfig mc) : Monitor(mc)
{
	if (!(mc.type & (stack)))
	{
		throw std::logic_error("Unhandled ADT: " + ext2str(mc.type));
	}
}

bool StackOptimalMonitor::ADT_supported(ADT adt)
{ return adt == ADT::stack; }

void StackOptimalMonitor::handle_push(event_t &e)
{
	history.add_push_call(e);
}

void StackOptimalMonitor::handle_ret_push(event_t &e)
{
	history.add_ret(e);
}

void StackOptimalMonitor::handle_pop(event_t &e)
{
	history.add_pop_call(e);
}

void StackOptimalMonitor::handle_ret_pop(event_t &e)
{
	history.add_ret(e);
}

void StackOptimalMonitor::print_state() const
{
	*output << history << std::endl;
}

void StackOptimalMonitor::do_linearization()
{
	// throw Violation("adad");

	try
	{
		std::map<val_t, CoverVal> values;
		for (auto &cv : history.values)
		{
			values.insert({cv.val, CoverVal(cv.val, cv.add, cv.rmv)});
		}
		if(values.empty())
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
			throw Violation("Ext empty, h not empty");
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
	catch(std::logic_error e)
	{
		throw Crash("logic error: " + ext2str(e.what()));
	}
}
