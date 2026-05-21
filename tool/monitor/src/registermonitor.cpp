#include "monitor/registermonitor.hpp"
#include "interval.h"
#include "exception.h"

using namespace std;

void RegisterMonitor::handle_write(event_t &e)
{
	try
	{
		add_time(e.timestamp);
		if (!e.val)
		{
			unlin = true;
			return;
		}
		val_t v = e.val.value();
		if (!values.contains(e.val.value()))
		{
			values[v] = RegValue();
		}
		values[v].write_call = e.timestamp;

		active[e.thread] = e;
	}
	catch (std::exception e)
	{
		throw Crash(e.what());
	}
}

void RegisterMonitor::handle_ret_write(event_t &e)
{
	add_time(e.timestamp);
	event_t &write_evt = active[e.thread];
	val_t v = write_evt.val.value();
	values[v].write_ret = e.timestamp;
	active.erase(e.thread);
}

void RegisterMonitor::handle_read(event_t &e)
{
	add_time(e.timestamp);
	active[e.thread] = e;
}

void RegisterMonitor::handle_ret_read(event_t &e)
{
	add_time(e.timestamp);
	event_t read_evt = active[e.thread];
	active.erase(e.thread);
	if (!e.val.has_value() && !read_evt.val.has_value())
	{
		// useless read. skip
		return;
	}
	val_t v = e.val.value_or(read_evt.val.value_or(-1));
	RegValue &rv = values[v];
	rv.min_read_call = min(rv.min_read_call, read_evt.timestamp);
	rv.min_read_ret = min(rv.min_read_ret, e.timestamp);
	rv.max_read_call = max(rv.max_read_call, read_evt.timestamp);
	rv.max_read_ret = max(rv.max_read_ret, e.timestamp);
}

void RegisterMonitor::handle_crash(event_t &e)
{
	add_time(e.timestamp);
	std::vector<event_t> events_to_handle;
	std::vector<bool> is_ret_write;
	for (auto &[t, call_evt] : active)
	{
		event_t ret_evt(Ereturn, t, {}, e.timestamp);
		events_to_handle.emplace_back(ret_evt);
		if (call_evt.type == Ewrite)
		{
			is_ret_write.push_back(true);
		}
		else if (call_evt.type == Eread)
		{
			is_ret_write.push_back(false);
		}
	}

	for(int i=0;i<is_ret_write.size();++i)
	{
		if(is_ret_write[i])
		{
			handle_ret_write(events_to_handle[i]);
		}
		else 
		{
			handle_ret_read(events_to_handle[i]);
		}
	}

	active.clear();
}

template <typename K, typename V>
V& getOrInsert(std::unordered_map<K, V>& mp, K& key)
{
	// if(mp.contains(key))
	// 	return mp[key];
	// else 
	// {
	// 	mp.insert({key, V()});
	// 	return mp[key];
	// }
	return mp[key]
}

void RegisterMonitor::do_linearization()
{
	// std::cout << "Start monitor\n";
	try
	{
		if (unlin)
			throw Violation("invalid register history");

		struct SweepPoint
		{
			bool has_forced_start = false;
			bool has_forced_end = false;
			bool has_flexible_start = false;
			bool has_flexible_end = false;

			timestamp_t forced_end = 0;
			timestamp_t flexible_end = 0;

		};

		std::unordered_map<timestamp_t, SweepPoint> pts;

		for (const auto &[v, rv] : values)
		{
			const bool has_write =
				(rv.write_call != 0 || rv.write_ret != 0);

			const bool has_read =
				(rv.min_read_ret != LLONG_MAX);

			if (has_read && !has_write)
				throw Violation("read without write");

			if (has_read && rv.min_read_ret < rv.write_call)
				throw Violation("read before write");

			if (!has_write && !has_read)
				continue;

			timestamp_t s_v = rv.write_call;
			timestamp_t e_v = rv.write_ret;

			if (has_read)
			{
				s_v = std::max(s_v, rv.max_read_call);
				e_v = std::min(e_v, rv.min_read_ret);
			}

			// Forced interval [e_v, s_v]
			if (s_v > e_v)
			{
				// cout << "Forced " << v << " " << AtomicInterval(true, e_v, s_v, true) << "\n";
				SweepPoint &sp1 = getOrInsert(pts, e_v);
				SweepPoint &sp2 = getOrInsert(pts, s_v);

				if (sp1.has_forced_start)
					throw Violation("forced start overlap");

				sp1.has_forced_start = true;
				sp1.forced_end = s_v;

				sp2.has_forced_end = true;
			}
			// Flexible interval [e_v, s_v]
			else
			{
				// cout << "Flex " << v << " " << AtomicInterval(true, s_v, e_v, true) << "\n";
				SweepPoint &sp1 = getOrInsert(pts, s_v);
				SweepPoint &sp2 = getOrInsert(pts, e_v);

				if (sp1.has_flexible_start)
					sp1.flexible_end = min(sp1.flexible_end, e_v);
				else
				{
					sp1.has_flexible_start = true;
					sp1.flexible_end = e_v;
				}

				sp2.has_flexible_end = true;
			}
		}

		bool forced_active = false;
		timestamp_t active_forced_end = 0;

		bool flex_active = true;
		timestamp_t min_flex_end = LLONG_MIN;

		for (timestamp_t t : times)
		{
			SweepPoint sp;
			if(pts.contains(t))
				sp = pts[t];

			if(pts.contains(t))
			{
				if (sp.has_forced_start)
				{
					// Existing forced interval still covers t.
					if (forced_active && active_forced_end > t)
						throw Violation("overlapping value intervals");

					forced_active = true;
					active_forced_end = sp.forced_end;
				}

				if (sp.has_flexible_start)
				{
					flex_active = true;
					min_flex_end = min(min_flex_end, sp.flexible_end);
				}
			}
			bool covered = forced_active && t <= active_forced_end;

			if (!covered)
			{
				flex_active = false;
				min_flex_end = LLONG_MIN;
			}

			if(pts.contains(t)) 
			{
				if (sp.has_flexible_end)
				{
					if (flex_active && t >= min_flex_end)
					{
						throw Violation("flexible covered by forced");
					}
				}

				if (sp.has_forced_end)
				{
					if (forced_active && active_forced_end == t)
					{
						forced_active = false;
					}
				}
			}
		}
	}
	catch (const Violation &)
	{
		throw;
	}
	catch (...)
	{
		throw Crash("register monitor exception");
	}
	// std::cout << "End monitor\n";
}

void RegisterMonitor::print_state() const {}

RegisterMonitor::RegisterMonitor(MonitorConfig mc) : Monitor(mc) {}