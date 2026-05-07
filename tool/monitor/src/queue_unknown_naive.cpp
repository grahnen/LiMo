#include "monitor/queue_naive.hpp"

void QueueNaiveUnknownMonitor::handle_enq(event_t &e)
{
	active[e.thread] = e;
	timestamps.push_back(e.timestamp);
	opEras[e.timestamp] = curEra;
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
}

void QueueNaiveUnknownMonitor::handle_deq(event_t &e)
{
	active[e.thread] = e;
	tempPopsIdx[e.timestamp] = tempPops.size();
	tempPops.push_back({AtomicInterval(true, e.timestamp, POSINF, false), e.val});
	timestamps.push_back(e.timestamp);
	opEras[e.timestamp] = curEra;
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
}

void QueueNaiveUnknownMonitor::handle_ret_enq(event_t &e)
{
	if (!active.contains(e.thread))
	{
		throw Violation("invalid history");
	}
	auto &call = active[e.thread];
	auto val = call.val.value_or(e.val.value_or(val_t()));
	QueueOp pushOp = {AtomicInterval(true, call.timestamp, e.timestamp, true), val};
	enqs.push_back(pushOp);
	timestamps.push_back(e.timestamp);
	opEras[e.timestamp] = curEra;
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
	active.erase(e.thread);
}

void QueueNaiveUnknownMonitor::handle_ret_deq(event_t &e)
{
	if (!active.contains(e.thread))
	{
		throw Violation("invalid history");
	}
	auto &call = active[e.thread];
	auto idx = tempPopsIdx[call.timestamp];
	tempPops[idx].itv = AtomicInterval(true, call.timestamp, e.timestamp, true);
	tempPops[idx].val = e.val.has_value() ? e.val : call.val;

	if (!tempPops[idx].val.has_value())
	{
		// std::cout << tempPops[idx].itv << " has no value\n";
		outers.push_back(AtomicInterval::closed(call.timestamp, e.timestamp));
		tempPops[idx].itv = AtomicInterval::open(POSINF, POSINF);
	}
	// std::cout << "found deq of " << tempPops[idx].val << " " << tempPops[idx].itv << "\n";
	timestamps.push_back(e.timestamp);
	opEras[e.timestamp] = curEra;
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
	active.erase(e.thread);
}

void QueueNaiveUnknownMonitor::handle_crash(event_t &e)
{
	curEra++;
	for (auto &[t, call] : active)
	{
		if (call.type == Edeq || call.type == Epop)
		{
			auto idx = tempPopsIdx[call.timestamp];
			tempPops[idx].itv = AtomicInterval(true, call.timestamp, e.timestamp, true);
			tempPops[idx].val = {};
		}
	}
	timestamps.push_back(e.timestamp);
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
	active.clear();
}

void QueueNaiveUnknownMonitor::handle_push(event_t &e)
{
	handle_enq(e);
}
void QueueNaiveUnknownMonitor::handle_pop(event_t &e)
{
	handle_deq(e);
}
void QueueNaiveUnknownMonitor::handle_ret_push(event_t &e)
{
	handle_ret_enq(e);
}
void QueueNaiveUnknownMonitor::handle_ret_pop(event_t &e)
{
	handle_ret_deq(e);
}

void QueueNaiveUnknownMonitor::simplify()
{
	// Add enough deqs at the end
	// std::cout << "adding " <<  enqs.size() << "deqs\n";
	timestamp_t mxT = maxTimestamp;
	for (timestamp_t t = mxT + 1; t <= mxT + enqs.size(); t++)
	{
		// std::cout <
		event_t deqCall(Edeq, t, {}, t);
		handle_deq(deqCall);
	}

	event_t crashE(Ecrash, 0, {}, maxTimestamp + enqs.size() + 1);
	handle_crash(crashE);

	for (auto &o : tempPops)
	{
		// std::cout << "deq " << o.itv << " " << o.val << "\n";
		if (o.val.has_value())
		{
			// std::cout << "deq value " << o.val.value() << "\n";
			deqs[o.val.value()] = o;
			hasPop[o.val.value()] = true;
		}
		else if (o.itv.empty())
		{
		}
		else
		{
			if(verbose)
				std::cout << "pending deq: " << o.itv << "\n";
			pendingDeqs.push_back(o);
		}
	}

	for (auto o : enqs)
	{
		auto val = o.val.value();
		if (hasPop.contains(val) && hasPop[val])
		{

			values[val] = {val, o, deqs[val]};
			opVals[o.itv.lbound] = val;
			opVals[o.itv.ubound] = val;
			opVals[deqs[val].itv.lbound] = val;
			opVals[deqs[val].itv.ubound] = val;

			if(verbose)
				std::cout << val << "-" << o.itv << deqs[val].itv << "\n";
		}
		else
		{
			if(verbose)
				std::cout << "pending Enq: " << o.itv << "\n";
			pendingEnqs.push_back(o);
		}
		// else
		// {
		// 	if(curPop == pendingdeqs.end())
		// 		throw Violation("Not enough deqs");

		// 	values.push_back({val, o, *curPop});
		// 	curPop++;
		// }
	}

	// compute queueEra counters
	if (timestamps.empty())
		return;

	queueCtr[timestamps[0]] = 0;
	auto curT = timestamps.begin() + 1;
	auto prevT = timestamps.begin();

	for (; curT != timestamps.end(); curT++, prevT++)
	{
		if (opVals.contains(*curT) && values[opVals[*curT]].pushOp.itv.ubound == *curT)
		{
			queueCtr[*curT] = std::max(queueCtr[*prevT], opEras[values[opVals[*curT]].popOp.itv.ubound]);
		}
		else
		{
			queueCtr[*curT] = queueCtr[*prevT];
		}
	}

	for (auto &o : pendingEnqs)
	{
		timestamp_t feasibleEra = max(opEras[o.itv.lbound], queueCtr[o.itv.lbound]);
		feasibleEras.push_back(feasibleEra);
	}
}

void QueueNaiveUnknownMonitor::do_linearization()
{
	simplify();

	timestamp_t num_pushes = pendingEnqs.size();
	timestamp_t num_pops   = pendingDeqs.size() - num_pushes;

	if(num_pops < num_pushes)
		throw Violation("not enough deqs");

	std::vector<int> perm;
	for(timestamp_t i = 0; i<num_pushes;i++)
		perm.push_back(i);
	for(timestamp_t i = 0;i<num_pops;i++)
		perm.push_back(-1);

	sort(perm.begin(), perm.end());

	for(auto &[v, o] : values)
	{
		inners.push_back(AtomicInterval::closed(o.pushOp.itv.ubound, o.popOp.itv.lbound));
		outers.push_back(AtomicInterval::closed(o.pushOp.itv.lbound, o.popOp.itv.ubound));
	}

	bool found = false;

	do
	{
		if(verbose)
			std::cout << "==========\n";
		std::vector<AtomicInterval> this_inner, this_outer;
		
		this_inner.insert(this_inner.end(), inners.begin(), inners.end());
		this_outer.insert(this_outer.end(), outers.begin(), outers.end());

		bool exit = false;
		for(timestamp_t i = 0; i <  perm.size(); i++)
		{
			if(perm.at(i) == -1)
				continue;
			auto pushOp = pendingEnqs.at(perm.at(i));
			auto popOp  = pendingDeqs.at(i);

			// std::cout << pushOp.itv << " " << popOp.itv << "\n";

			auto outer_itv = AtomicInterval::closed(pushOp.itv.lbound, popOp.itv.ubound);
			if(outer_itv.empty())
			{
				exit = true;
				break;
			}
			if(verbose)
			std::cout << "pending: i/o" << AtomicInterval::closed(pushOp.itv.ubound, popOp.itv.lbound) << outer_itv << "\n";

			this_inner.push_back(AtomicInterval::closed(pushOp.itv.ubound, popOp.itv.lbound));
			this_outer.push_back(outer_itv);

			// queue_tree.deleteTree(queue_tree.root);
		}

		if(exit)
			continue;
		queue_tree.deleteTree(queue_tree.root);
		queue_tree.root = nullptr;
		for(auto &itv : this_inner)
		{
			// std::cout << "inserting " << itv << "\n";
			ItvNode n = ItvNode(itv);
			queue_tree.insert(n);
		}
		if(queue_tree.root != nullptr)
			compute_k(queue_tree.root);
		if(verbose)
		{
			queue_tree.printTree();
		}
		

		auto n = queue_tree.root;


		found = true;
		for (auto vl : this_outer)
		{
			if(verbose)
			{
				std::cout << "Searching: " << vl << "\n";
			}
			if (contains(n, vl))
			{
				found = false;
				break;
			}
		}
		if(found)
		{
			if(verbose)
			{
				std::cout << "found assignment: \n";
				for(auto i : perm)
					cout << i << " ";
			}
			break;
		}
	} while (next_permutation(perm.begin(), perm.end()));
	
	if(!found)
	{
		throw Violation("no linearization found");
	}
	
}

void QueueNaiveUnknownMonitor::print_state() const
{
}

QueueNaiveUnknownMonitor::QueueNaiveUnknownMonitor(MonitorConfig mc) : Monitor(mc),
															 curEra(0),
															 maxTimestamp(0)
{
}