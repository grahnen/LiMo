#include "monitor.hpp"
#include <iostream>
#include "exception.h"
#include "util.h"
#include <map>
#include <string.h>

#define BRANCH(r, data, elem) if(data == BOOST_PP_CAT(E,elem)) { BOOST_PP_CAT(handle_,elem)(e); } else
#define BRANCH_RET(r, data, elem) if(data == BOOST_PP_CAT(E,elem)) { BOOST_PP_CAT(handle_ret_,elem)(e); } else

void Monitor::add_event(event_t &e) {
  if (verbose) {
    *output << e.timestamp << ": " << e << std::endl;
  }
  if (e.type == Ereturn) {
    if(!lastEvt.contains(e.thread) || lastEvt[e.thread] == Ereturn)
      throw Exception("Return before Call, error in history");

    BOOST_PP_SEQ_FOR_EACH(BRANCH_RET,lastEvt[e.thread],OP_TYPE_SEQ);

    lastEvt[e.thread] = e.type;

    rem_conc();

  } else {
    if(e.type == Ecrash)
      lastEvt.clear();

    if(lastEvt.contains(e.thread) && lastEvt[e.thread] != Ereturn)
      throw Exception("Multiple calls before return, error in history");

    lastEvt[e.thread] = e.type;

    BOOST_PP_SEQ_FOR_EACH(BRANCH,e.type,OP_TYPE_SEQ)
    {
      // No match
      if(e.type == Ecrash) {
        handle_crash(e);
      } else {
        throw std::runtime_error("Unhandled event: " + etype_name[e.type]);
      }
    }
    add_conc();
  }

  if(verbose)
    print_state();
}

void Monitor::set_verbose(bool v) {
  verbose = v;
}

void Monitor::add_conc() {
  nConc++;
  if(nConc > maxConc)
    maxConc = nConc;

  totConc += nConc;
  nOps++;
}

void Monitor::rem_conc() {
  nConc--;
  totConc += nConc;
  nOps++;
}

int Monitor::max_conc() { return maxConc; }

float Monitor::avg_conc() { return ((float)totConc) / nOps; }
