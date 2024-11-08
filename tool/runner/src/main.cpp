#include <iostream>
#include <vector>
#include "pthread.h"
#include <iostream>
#include <random>
#include "io.h"
#include "impl.h"
#include "event.h"
#include <cassert>
#include <fstream>
#include <ctime>

#include "convert.h"
#include <chrono>
#include <boost/program_options.hpp>

#include "algorithm.hpp"
#include "suite.hpp"
#include "results.hpp"

#define UID (data->num_ops * data->threads + i);


using namespace boost::program_options;

std::string mode;
std::string algorithm;
std::string filename;
std::string suite;
bool verbose = false;
bool write_logs;
int reps;
int increment;

using cl = std::chrono::steady_clock;

bool run = false;

bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
      ("verbose,v", value(&verbose)->default_value(false)->implicit_value(true), "Verbose")
      ("mode,m", value(&mode)->default_value("generate")->implicit_value("benchmark"), "Mode")
      ("increment,i", value(&increment)->default_value(0)->implicit_value(1))
      ("algorithm,a", value(&algorithm)->default_value("segment")->implicit_value("segment"), "Algorithm (only for benchmark)")
      ("repetitions,r", value(&reps)->default_value(1)->implicit_value(10), "Number of repetitions")
      ("suite", value(&suite)->required())

      ("output", value(&filename)->required(), "Output file");

    positional_options_description pos;
    pos.add("suite", 1);
    pos.add("output", 2);
    variables_map vm;
    store(command_line_parser(argc, argv)
          .positional(pos)
          .options(desc)
          .run(), vm);
    notify(vm);
  } catch (const boost::program_options::error &ex) {
    std ::cerr << ex.what() << std::endl;
    return false;
  }
  return true;

}

struct event_placeholder {
  cl::time_point ts;
  EType type;
  optval_t v;
  tid_t tid;

  bool operator<(const event_placeholder &o) const {
    return ts < o.ts;
  }
};

event_placeholder *events;

struct data_t {
  ADTImpl *adt;
  int *values;
  int num_ops;
  tid_t tid;
};

void *thread_fn(void *arg) {
  data_t *data = (data_t *) arg;
  tid_t tid = data->tid;
  int c_val = 0;
  int n = 0;
  int ctr = 0;
  while (run == false) {
  }
  for(index_t i = 0; i < data->num_ops; i++) {
    tid_t uid = data->num_ops * tid + i;
    cl::time_point c = cl::now();
    val_t v = data->values[i];
    if(v == -1) {
      // Pop
      //

      events[(2 * tid * data->num_ops) + (2 * i)] = event_placeholder {
      .ts = c,
      .type = Epop,
      .v = {},
      .tid = uid
    };

      res_t vv = data->adt->rmv(tid);

      cl::time_point r = cl::now();
      events[(2 * tid * data->num_ops) + (2 * i) + 1] = event_placeholder {
      .ts = r,
      .type = Ereturn,
      .v = vv,
      .tid = uid,
    };




    } else if(v == 1) {
      //Push
      val_t val(tid, n++);


      events[(2 * tid * data->num_ops) + (2 * i)] = event_placeholder {
      .ts = c,
      .type = Epush,
      .v = val,
      .tid = uid,
    };
      ctr++;
      data->adt->add(val, tid);

      cl::time_point r = cl::now();
      events[(2 * tid * data->num_ops) + (2 * i) + 1] = event_placeholder {
      .ts = r,
      .type = Ereturn,
      .v = std::nullopt,
      .tid = uid,
    };

      

    } else {
      throw std::logic_error("Unknown value");
    }



  }

  delete data;
  pthread_exit(nullptr);
}


bool list_valid(int *ops, int n_ops) {
  int s = 0;
  for(int i = 0; i < n_ops; i++) {
    s += ops[i];
    if(s < 0)
      return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  if(!get_options(argc, argv)){
    std::cout << "Invalid arguments" << std::endl;
    return -1;
  }

  std::vector<config> confs = load_suite(suite, increment);
  std::map<config,std::vector<double>> results;
  for(auto cfg : confs) {
    int threads = cfg.threads;
    int pushes = cfg.values;
    for(int i = 0; i < reps; i++) {
      std::cout << threads << " threads, " << pushes << " adds." << std::endl;
      int num_vals = pushes / threads;
      int num_pops = (pushes * 0.8) / threads;
      int ops_per_thread = num_vals + num_pops;

      int num_ops = (ops_per_thread) * threads;

      events = new event_placeholder[num_ops * 2];

      int **array = new int*[threads];

      std::random_device rd;
      std::mt19937_64 g(rd());

      for(int t = 0; t < threads; t++) {
        array[t] = new int[ops_per_thread];
        for(int i = 0; i < num_vals; i++) {
          array[t][i] = 1;
        }
        for(int i = 0; i < num_pops; i++) {
          array[t][num_vals + i] = -1;
        }

        // There must be at least as many pushes as pops at each point in time..
        std::shuffle(&array[t][0], &array[t][ops_per_thread], g);


        while(!list_valid(array[t], ops_per_thread)) {
          std::shuffle(&array[t][0], &array[t][ops_per_thread], g);
        }

      }

      //Monitor *m = (Monitor *) new ADTImplMonitor();
      ADTImpl *s = new ADTImpl(threads);

      pthread_t thread_arr[threads];
      //events = new event_t*[num_threads];

      for (tid_t i = 0; i < threads; i++) {
        data_t *data = new data_t {
        .adt = s,
        .values = array[i],
        .num_ops = ops_per_thread,
        .tid = i
        };
        //events[i] = new event_t[ops_per_thread * 2];

        int t = pthread_create(&thread_arr[i], NULL, thread_fn, (void *) data);
      }
      run = true;
      for (int i = 0; i < threads; i++) {
        pthread_join(thread_arr[i], nullptr);
      }




      std::string adt;
      if(s->adt == ADT::stack) {
        adt = "atomic-stack";
      } else {
        adt = "atomic-queue";
      }

      delete s;

      std::sort(&events[0], &events[num_ops * 2]);
      std::vector<event_t> evts;
      evts.reserve(num_ops * 2);

      timestamp_t ts = 1;

      for(int i = 0; i < num_ops * 2; i++) {
        tid_t t = events[i].tid;
        event_t e(events[i].type, t, events[i].v, ts++);
        evts.push_back(e);
      }
      write_file(&evts, adt, filename + ext2str(cfg) + (reps == 0 ? "" : "r" + ext2str(i)) + ".hist");
      delete[] events;
      for(int i = 0; i < threads; i++){
        delete[] array[i];
      }
      delete[] array;
    }
  }
}
