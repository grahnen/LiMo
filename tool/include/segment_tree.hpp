#ifndef _SEGMENT_TREE_H_
#define _SEGMENT_TREE_H_

#include <interval.h>
#include "typedef.h"
#include <vector>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>
#include "exception.h"
// This can be optimized to return larger intervals

class tsSegmentTree
{
	std::size_t arrSize;
	std::size_t treeSize;
	std::size_t arrBase;
	std::size_t levels;
	std::size_t maxIdx;
	std::vector<long long int> tree;
	std::vector<long long int> updateArr;
	std::map<timestamp_t, std::size_t> timestampsInv;
	std::vector<timestamp_t> timestamps;
	std::vector<AtomicInterval> minIntervals;

	bool verbose;
	std::ostream *output;

public:
	// n number of timestamps
	tsSegmentTree(std::vector<timestamp_t> &tsVec) : verbose(false),
													 output(&std::cout)
	{
		std::size_t n = tsVec.size();
		arrSize = 1 << (long long int)ceil(log2(n - 1));
		treeSize = 2 * arrSize;
		tree.resize(treeSize, 0);
		updateArr.resize(treeSize, 0);
		minIntervals.resize(treeSize, AtomicInterval::nil());
		arrBase = arrSize;
		levels = ceil(log2(n - 1));
		maxIdx = n - 1;
		sort(tsVec.begin(), tsVec.end());

		for (std::size_t i = 0; i < tsVec.size(); i++)
		{
			timestampsInv[tsVec.at(i)] = i;
		}
		timestamps = tsVec;

		for (int i = 0; i < arrSize; i++)
		{
			// std::cout << i << " ";
			timestamp_t l, r;
			tsFromIdx(i, i, l, r);
			minIntervals.at(i + arrBase) = AtomicInterval(true, l, r, true);
		}
		// std::cout<<"\n";
		for (int i = arrBase - 1; i >= 0; i--)
		{
			minIntervals.at(i) = minIntervals.at(2 * i);
		}
	}

	void set_verbose(bool v)
	{
		verbose = v;
	}

	void set_output(std::ostream &os)
	{
		output = &os;
	}

	// get element at index idx
	long long int element(int idx)
	{
		return query(idx, idx);
	}

	void print(std::ostream &cout = std::cout)
	{

		for (int i = 1; i < treeSize; i++)
		{
			pushUpdate(i);
		}
		for (auto it = tree.begin() + arrBase; it != tree.end(); it++)
			cout << *it << " ";
		cout << "\n";
	}

	// initialize the Segment Tree
	// void init(std::vector<long long int> arr)
	// {
	// 	for (int i = 0; i < std::min(arr.size(), treeSize); i++)
	// 	{
	// 		update(i, arr.at(i));
	// 	}
	// }

	// update range [ts1,ts2] by off
	void update(timestamp_t ts1, timestamp_t ts2, long long int off)
	{
		std::size_t l, r;
		indexFromTs(ts1, ts2, l, r);
		if (verbose)
		{
			*output << "SegTree: Updating indices [" << l << "," << r << "]" << std::endl;
		}
		updateRec(l, r + 1, off, 1, levels);
	}

	inline void update(const AtomicInterval &itv, long long int off)
	{
		update(itv.lbound, itv.ubound, off);
	}

	// query range [ts1,ts2]
	long long int query(timestamp_t ts1, timestamp_t ts2)
	{
		std::size_t l, r;
		indexFromTs(ts1, ts2, l, r);
		if (ts1 >= ts2)
			return LLONG_MAX;
		if (verbose)
		{
			*output << "SegTree: Querying indices [" << l << "," << r << "]" << std::endl;
		}
		return queryRec(l, r + 1, 1, levels);
	}

	inline long long int query(const AtomicInterval &itv)
	{
		if (verbose)
		{
			long long int x = query(itv.lbound, itv.ubound);
			*output << "SegTree: Query returning " << x << std::endl;
			return x;
		}
		return query(itv.lbound, itv.ubound);
	}

	// query range [ts1, ts2]
	AtomicInterval getMinInterval(timestamp_t ts1, timestamp_t ts2, long long int &minVal)
	{
		minVal = LLONG_MAX;
		if (ts1 >= ts2)
		{
			minVal = LLONG_MAX;
			return AtomicInterval::nil();
		}
		std::size_t l, r;
		indexFromTs(ts1, ts2, l, r);
		if (verbose)
		{
			*output << "SegTree: Interval Querying indices [" << l << "," << r << "]" << std::endl;
		}
		return queryRecItv(l, r + 1, 1, levels, minVal);
	}

	inline AtomicInterval getMinInterval(const AtomicInterval &itv, long long int &minVal)
	{
		if (verbose)
		{
			auto x = getMinInterval(itv.lbound, itv.ubound, minVal);
			*output << "SegTree: Query returning " << x << "-" << minVal << std::endl;
			return x;
		}
		return getMinInterval(itv.lbound, itv.ubound, minVal);
	}

	long long int elementRight(timestamp_t ts)
	{

		size_t idx, idx2;
		indexFromTs(ts, timestamps.back(), idx, idx2);
		return queryRec(idx, idx+1, 1, levels);
	}

private:
	// returns range [idx1, idx2] for ts [ts1,ts2]
	void indexFromTs(timestamp_t ts1, timestamp_t ts2, size_t &idx1, size_t &idx2)
	{
		// cout<<"indexFromTs " << ts1 <<" " << ts2 <<"\n";
		if (!timestampsInv.contains(ts1))
			throw Exception("timestamp index out of range: " + std::to_string(ts1));
		idx1 = timestampsInv.at(ts1);
		if (!timestampsInv.contains(ts2))
			throw Exception("timestamp index out of range: " + std::to_string(ts2));
		idx2 = timestampsInv.at(ts2) - 1;
	}

	// returns range [ts1,ts2] from [idx1,idx2]
	void tsFromIdx(size_t idx1, size_t idx2, timestamp_t &ts1, timestamp_t &ts2)
	{
		if(idx1 >= timestamps.size())
			ts1 = timestamps[timestamps.size()-1];
		else
			ts1 = timestamps.at(idx1);
		if(idx2+1 >= timestamps.size())
			ts2 = timestamps[timestamps.size()-1];
		else
			ts2 = timestamps.at(idx2 + 1);
	}

	// pu	sh udpate down one step
	void pushUpdate(std::size_t node)
	{
		if (node >= treeSize)
			return;
		if (node < arrBase)
		{
			updateArr.at(2 * node) += updateArr.at(node);
			updateArr.at(2 * node + 1) += updateArr.at(node);
			tree.at(2 * node) += updateArr.at(node);
			tree.at(2 * node + 1) += updateArr.at(node);
		}
		updateArr.at(node) = 0;
	}

	// updates range array idx [l,r)
	void updateRec(std::size_t l, std::size_t r, long long int off, std::size_t node, std::size_t h)
	{
		pushUpdate(node);
		if (h == 0)
		{
			tree.at(node) += off;
			return;
		}
		int crit = 1 << (h - 1);
		if (l == 0 && r == (2 * crit))
		{
			updateArr.at(node) += off;
			tree.at(node) += off;
		}
		else
		{
			// tree.at(node) += off;
			if (l >= crit)
			{
				updateRec(l - crit, r - crit, off, 2 * node + 1, h - 1);
			}
			else if (r <= crit)
			{
				updateRec(l, r, off, 2 * node, h - 1);
			}
			else
			{
				updateRec(l, crit, off, 2 * node, h - 1);
				updateRec(0, r - crit, off, 2 * node + 1, h - 1);
			}

			std::size_t lVal = tree[2 * node], rVal = tree[2 * node + 1];
			if (lVal <= rVal)
			{
				minIntervals[node] = minIntervals[2 * node];
			}
			else
			{
				minIntervals[node] = minIntervals[2 * node + 1];
			}
			tree.at(node) = min(tree.at(2 * node), tree.at(2 * node + 1));
		}
	}

	// queries range arr idx [l,r)
	long long int queryRec(std::size_t l, std::size_t r, std::size_t node, std::size_t h)
	{
		pushUpdate(node);
		if (h == 0)
		{
			return tree.at(node);
		}
		int crit = 1 << (h - 1);
		if (l == 0 && r == (2 * crit))
			return tree.at(node);
		else
		{
			if (l >= crit)
			{
				return queryRec(l - crit, r - crit, 2 * node + 1, h - 1);
			}
			else if (r <= crit)
			{
				return queryRec(l, r, 2 * node, h - 1);
			}
			else
			{
				return std::min(queryRec(l, crit, 2 * node, h - 1), queryRec(0, r - crit, 2 * node + 1, h - 1));
			}
		}
	}

	// queries range arr idx [l,r) and returns first min interval
	AtomicInterval queryRecItv(std::size_t l, std::size_t r, std::size_t node, std::size_t h, long long int &minVal)
	{
		pushUpdate(node);
		if (h == 0)
		{
			minVal = tree.at(node);
			return minIntervals.at(node);
		}
		int crit = 1 << (h - 1);
		if (l == 0 && r == (2 * crit))
		{
			minVal = tree.at(node);
			return minIntervals.at(node);
		}
		else
		{
			if (l >= crit)
			{
				return queryRecItv(l - crit, r - crit, 2 * node + 1, h - 1, minVal);
			}
			else if (r <= crit)
			{
				return queryRecItv(l, r, 2 * node, h - 1, minVal);
			}
			else
			{
				AtomicInterval left, right;
				long long int min1, min2;

				left = queryRecItv(l, crit, 2 * node, h - 1, min1);
				right = queryRecItv(0, r - crit, 2 * node + 1, h - 1, min2);

				if (min1 <= min2)
				{
					minVal = min1;
					return left;
				}
				else
				{
					minVal = min2;
					return right;
				}
			}
		}

		throw Exception("no return - queryRecItv");
		return AtomicInterval();
	}
};

#endif