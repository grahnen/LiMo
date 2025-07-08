#ifndef NAIVE_DURABLE_STACK_H_
#define NAIVE_DURABLE_STACK_H_


#include "event.h"
#include "monitor.hpp"
#include "coverhistory.h"
#include "typedef.h"
#include "monitor/stackUnknownMonitor.hpp"


class NaiveDurableStackMonitor : public Monitor {

protected:
	index_t height = 0;
	using LinRes = CoverHistory::LinRes;
	CoverHistory history;
	DECLHANDLER(push)
	DECLHANDLER(pop)
	void handle_crash(event_t &);
public:
	void do_linearization();
	NaiveDurableStackMonitor(MonitorConfig mc);
	void print_state() const;
	bool ADT_supported(ADT adt) { return adt == stack; }
};

class NaiveDStackUnknown : public StackUnknownMonitor
{
	//inherit handlers from parent
	//inherit CheckStackLin

public:
	NaiveDStackUnknown(MonitorConfig config);
	void do_linearization() override;
};


#endif // NAIVE_DURABLE_STACK_H_
