#include "monitor/queue_unknownmonitor.hpp"

void QueueUnknownMonitor::handle_enq(event_t &e)
{
	active[e.thread] = e;
	timestamps.push_back(e.timestamp);
	opEras[e.timestamp] = curEra;
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
	if(e.val.has_value())
	{
		opVals[e.timestamp] = e.val.value();
	}
}

void QueueUnknownMonitor::handle_deq(event_t &e)
{
	active[e.thread] = e;
	tempPopsIdx[e.timestamp] = tempPops.size();
	tempPops.push_back({AtomicInterval(true, e.timestamp, POSINF, false), e.val});
	timestamps.push_back(e.timestamp);
	opEras[e.timestamp] = curEra;
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
	if(e.val.has_value())
	{
		opVals[e.timestamp] = e.val.value();
	}
}

void QueueUnknownMonitor::handle_ret_enq(event_t &e)
{
	if (!active.contains(e.thread))
	{
		throw Violation("invalid history");
	}
	auto &call = active[e.thread];
	auto val = call.val.value_or(e.val.value_or(val_t()));
	opVals[e.timestamp] = val;
	opVals[call.timestamp] = val;
	QueueOp pushOp = {AtomicInterval(true, call.timestamp, e.timestamp, true), val};
	enqs.push_back(pushOp);
	timestamps.push_back(e.timestamp);
	opEras[e.timestamp] = curEra;
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
	active.erase(e.thread);
}

void QueueUnknownMonitor::handle_ret_deq(event_t &e)
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

void QueueUnknownMonitor::handle_crash(event_t &e)
{
	curEra++;
	// cout << active.size() << "events crashed\n";
	for (auto &[t, call] : active)
	{
		// std::cout << t<<"-"<<call<<"\n";
		if (call.type == Edeq || call.type == Epop)
		{
			auto idx = tempPopsIdx[call.timestamp];
			tempPops[idx].itv = AtomicInterval(true, call.timestamp, e.timestamp, true);
			tempPops[idx].val = {};

			// std::cout << "Crashed pop: " << tempPops[idx].itv << "\n";
		}
		else if(call.type == Eenq || call.type == Epush)
		{
			if(!call.val.has_value())
				throw Violation("push without value");
			crashedEnqs[call.val.value()] = QueueOp{AtomicInterval::closed(call.timestamp, e.timestamp), call.val};
		}
	}
	timestamps.push_back(e.timestamp);
	maxTimestamp = std::max(maxTimestamp, e.timestamp);
	active.clear();
}

void QueueUnknownMonitor::handle_push(event_t &e)
{
	handle_enq(e);
}
void QueueUnknownMonitor::handle_pop(event_t &e)
{
	handle_deq(e);
}
void QueueUnknownMonitor::handle_ret_push(event_t &e)
{
	handle_ret_enq(e);
}
void QueueUnknownMonitor::handle_ret_pop(event_t &e)
{
	handle_ret_deq(e);
}

void QueueUnknownMonitor::simplify()
{
	// Add enough deqs at the end
	// std::cout << "adding " <<  enqs.size() << "enqs\n";
	timestamp_t mxT = maxTimestamp;
	for (timestamp_t t = mxT + 1; t <= mxT + enqs.size(); t++)
	{
		// std::cout <
		event_t deqCall(Edeq, t, {}, t);
		handle_deq(deqCall);
	}

	event_t crashE(Ecrash, 0, {}, maxTimestamp + enqs.size() + 1);
	handle_crash(crashE);

	// std::cout << "=============\n";
	// std::cout << "Total eras: " << curEra << "\n";

	pendingDeqs.resize(curEra);

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
			{
				std::cout << "Pending Deq: " << o.itv << " in era" << opEras[o.itv.lbound] << "\n";
			}
			pendingDeqs[opEras[o.itv.lbound]].push(o);
		}
	}

	// std::cout << "pending deqs sizes: ";
	// for(auto &v : pendingDeqs)
	// {
	// 	std::cout << v.size() << " ";
	// }	
	// std::cout << "\n";

	for (auto o : enqs)
	{
		auto val = o.val.value();
		if (hasPop.contains(val) && hasPop[val])
		{
			// std::cout << "Addind to values: " << val << o.itv << deqs[val].itv << "\n";
			values[val] = {val, o, deqs[val]};
			opVals[o.itv.lbound] = val;
			opVals[o.itv.ubound] = val;
			opVals[deqs[val].itv.lbound] = val;
			opVals[deqs[val].itv.ubound] = val;

			// std::cout << val << "-" << o.itv << deqs[val].itv << "\n";
		}
		else
		{
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
	for(auto &[val,o] : crashedEnqs)
	{
		if (hasPop.contains(val) && hasPop[val])
		{
			// std::cout << "Addind to values: " << val << o.itv << deqs[val].itv << "\n";
			values[val] = {val, o, deqs[val]};
			opVals[o.itv.lbound] = val;
			opVals[o.itv.ubound] = val;
			opVals[deqs[val].itv.lbound] = val;
			opVals[deqs[val].itv.ubound] = val;

			// std::cout << val << "-" << o.itv << deqs[val].itv << "\n";
		}
	}

	// compute queueEra counters
	if (timestamps.empty())
		return;

	queueCtr[timestamps[0]] = 0;
	auto curT = timestamps.begin() + 1;
	auto prevT = timestamps.begin();


	for (; curT != timestamps.end(); curT++, prevT++)
	{
		if(opVals.contains(*curT))
		{
			// std::cout << *curT << "-" << opVals[*curT] << "-" << values[opVals[*curT]].pushOp.itv << values[opVals[*curT]].popOp.itv << "\n"; 
		}
		if (opVals.contains(*curT) && values[opVals[*curT]].pushOp.itv.ubound == *curT)
		{
			queueCtr[*curT] = std::max(queueCtr[*prevT], opEras[values[opVals[*curT]].popOp.itv.ubound]);
		}
		else
		{

			queueCtr[*curT] = queueCtr[*prevT];
		}
	}

	// for(auto &[t, i] : queueCtr)
	// 	std::cout << i << "\n";

	for (auto &o : pendingEnqs)
	{
		timestamp_t feasibleEra = max(opEras[o.itv.lbound], queueCtr[o.itv.lbound]);
		feasibleEras.push_back(feasibleEra);
	}

	// cout << "fEras: ";
	// for(auto f : feasibleEras)
	// {
	// 	cout << f << "\n";
	// }
}

timestamp_t find(vector<timestamp_t> &parent, timestamp_t x)
{
	if (parent[x] != x)
	{
		parent[x] = find(parent, parent[x]);
	}
	return parent[x];
}

void merge(vector<timestamp_t> &parent,
		   vector<timestamp_t> &size,
		   vector<timestamp_t> &index,
		   timestamp_t x,
		   timestamp_t y)
{
	timestamp_t px = find(parent, x);
	timestamp_t py = find(parent, y);

	if (size[px] < size[py])
	{
		parent[px] = py;
		size[py] += size[px];
		index[py] = max(index[py], index[px]);
	}
	else
	{
		parent[py] = px;
		size[px] += size[py];
		index[px] = max(index[px], index[py]);
	}
}

void QueueUnknownMonitor::do_linearization()
{
	simplify();

	// do assignment -- add to values
	// check lin

	vector<timestamp_t> parent;
	vector<timestamp_t> size;
	vector<timestamp_t> index;

	// std::cout << "total eras: " << curEra << "\n";

	// std::cout << "num pops: ";

	for (int i = 0; i < curEra; i++)
	{
		parent.push_back(i);
		size.push_back(1);
		index.push_back(i);

		// std::cout << pendingDeqs[i].size() << " ";
	}
	// std::cout << "\n";

	size_t enqIdx = 0;
	for (auto &enq : pendingEnqs)
	{
		if (!enq.val.has_value())
			throw Violation("enq without value");
		auto val = enq.val.value();
		// find deq

		timestamp_t fEra = feasibleEras[enqIdx];
		timestamp_t aEra = index[find(parent, fEra)];

		// std::cout << "aEra = " << aEra << " " << pendingDeqs[aEra].size() <<"\n";

		while (pendingDeqs[aEra].empty())
		{
			if (aEra + 1 >= curEra)
			{
				throw Violation("no more deqs");
			}
			merge(parent, size, index, aEra, aEra + 1);
			aEra = index[find(parent, fEra)];
		}
		QueueOp deq = pendingDeqs[aEra].front();
		pendingDeqs[aEra].pop();
		if (pendingDeqs[aEra].empty())
		{
			if (aEra + 1 < curEra)
			{
				// throw Violation("no more deqs");
				merge(parent, size, index, aEra, aEra + 1);
			}
		}

		values[val] = {val, enq, deq};
		opVals[enq.itv.lbound] = val;
		opVals[enq.itv.ubound] = val;
		opVals[deq.itv.lbound] = val;
		opVals[deq.itv.ubound] = val;

		// std::cout << val << "-" << enq.itv << " " << deq.itv << "\n";
		enqIdx++;
	}

	std::vector<AtomicInterval> outers;

	for (auto &[v, evs] : values)
	{
		auto inner = AtomicInterval::closed(evs.pushOp.itv.ubound, evs.popOp.itv.lbound);
		auto outer = AtomicInterval::closed(evs.pushOp.itv.lbound, evs.popOp.itv.ubound);

		ItvNode n = ItvNode(inner);
		queue_tree.insert(n);

		if(verbose)
		{
			std::cout << v << ": " << inner << " " << outer << "\n";
		}

		outers.push_back(outer);
	}

	ItvTree::Node *n = queue_tree.root;
	compute_k(n);

	if(verbose)
		queue_tree.printTree();
	for (auto vl : outers)
	{
		if(verbose)
			std::cout << "checking: " << vl << "\n";
		if (contains(n, vl))
		{
			// std::cout << vl << "is violation";
			throw Violation("Unlinearizable: outer in cover!");
		}
	}
}

void QueueUnknownMonitor::print_state() const
{
}

QueueUnknownMonitor::QueueUnknownMonitor(MonitorConfig mc) : Monitor(mc),
															 curEra(0),
															 maxTimestamp(0)
{
}
