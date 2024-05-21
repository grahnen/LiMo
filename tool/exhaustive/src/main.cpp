#include <iostream>
#include "convert.h"
#include "io.h"
#include <boost/program_options.hpp>
#include "generator.hpp"
#include "execution.hpp"
#include "algorithm.hpp"
#include <signal.h>

#include "progressbar.hpp"

#define MESSAGE_STATUS 1
#define MESSAGE_COUNT 2
#define MESSAGE_MISMATCH 3
#define MESSAGE_DATA 4
#define MESSAGE_CRASH 5

#define STATUS_DONE 0
#define STATUS_ERR 1
#define STATUS_REQUEST 2

#include <mpi.h>

using namespace boost::program_options;

#define MPI_IDX MPI_UNSIGNED_LONG_LONG
#define MPI_THR MPI_UNSIGNED_LONG_LONG

bool verbose;

Algorithm alg;
Algorithm cmp;
index_t n_elements;
int num_fixed;
int num_sent;
tid_t max_thr;

int gen_counts(int rank, int size, int num_inits) {
  int even_div = num_inits / (size - 1);
  int remainder = num_inits % (size - 1);

  return even_div + ((rank - 1) < remainder ? 1 : 0);

}

bool interrupted = false;


static void sigintHandler(int sig) {
  switch(sig){
    case SIGINT:
      interrupted = true;
    default:
      std::cout << "Signal received: " << sig << std::endl;
  }
}


bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
      ("verbose,v", value(&verbose)->default_value(false)->implicit_value(true), "Verbose")
      ("algorithm,a", value(&alg)->default_value(Algorithm::segment), "Algorithm")
      ("compare,c", value(&cmp)->default_value(Algorithm::cover), "Comparison Algorithm")
      ("max_thr,t", value(&max_thr)->default_value(MAX_THREADS), "Maximum thread count")
      ("size", value(&n_elements)->required(), "Number of values");

    positional_options_description pos;
    pos.add("size", 1);
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


int main(int argc, char *argv[]) {
  signal(SIGTERM, sigintHandler);
  MPI_Init(&argc, &argv);
  int size, rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int n_inits;
  int num_each;
  int *counts = 0;

  int *data = 0;
  int *displs = 0;

  std::vector<std::vector<int>> gens;

  if(rank == 0) {
    // Root process handles input options.
    if(!get_options(argc, argv)) {
      throw std::logic_error("Invalid arguments");
    }

    std::cout << "Exhaustively testing histories with " << n_elements << " elements";
    if(max_thr < MAX_THREADS) {
      std:: cout << " on " << max_thr << " threads.";
    }
    std::cout << std::endl;
    std::cout << "Testing algorithm:\t" << alg << std::endl;
    std::cout << "Comparison algorithm:\t" << cmp << std::endl;
    // Root process should be communication hub instead of a worker.
    // Make more than needed to lessen the impact of different inits generating different history counts
    gens = create_inits(n_elements, (size - 1) * (size - 1) * 2, max_thr);

    num_each = gens[0].size();
    n_inits = gens.size();
    std::cout << (size - 1) << " processes gives " << n_inits << " inits" << std::endl;
    // if(n_inits < (size -1))
    //   throw std::logic_error("Cannot have more processes than possible histories");

    // data = new int[n_inits * num_each];
    // displs = new int[size];

    // for(int i = 0; i < gens.size(); i++) {
    //   for(int l = 0; l < gens[i].size(); l++) {
    //     if(gens[i][l] >= n_elements || gens[i][l] < 0)
    //       throw std::logic_error("Error in creation");
    //     data[i * num_each + l] = gens[i][l];
    //   }
    // }
    // counts = new int[size];
    // long long tot_cnt = 0;
    // for(int i = 1; i < size; i++) {
    //   int cnts = 1; //gen_counts(i, size, n_inits);
    //   counts[i] = cnts * num_each;
    //   tot_cnt += cnts;
    // }
    // counts[0] = 0;

    // int curr_displ = 0;
    // for(int i = 0; i < size; i++) {
    //   displs[i] = curr_displ;
    //   curr_displ += counts[i];
    // }
  }

  MPI_Bcast(&alg, 1, MPI_INT, 0, MPI_COMM_WORLD);

  MPI_Bcast(&cmp, 1, MPI_INT, 0, MPI_COMM_WORLD);

  MPI_Bcast(&max_thr, 1, MPI_THR, 0, MPI_COMM_WORLD);

  MPI_Bcast(&n_elements, 1, MPI_IDX, 0, MPI_COMM_WORLD);

//  MPI_Bcast(&n_inits, 1, MPI_INT, 0, MPI_COMM_WORLD);

  MPI_Bcast(&num_each, 1, MPI_INT, 0, MPI_COMM_WORLD);

  int *init = rank == 0 ? nullptr : new int[num_each];

  MPI_Request request;

  long long linearizable = 0;
  long long violations = 0;
  long long mismatches = 0;
  long long num_finished = 0;
  if(rank == 0) {
    // Root process does non-work related tasks. (mainly storing mismatches and receiving messages.).
    delete[] counts;
    delete[] displs;
    
    num_sent = 0;
    std::vector<bool> thread_done;
    thread_done.resize(size-1, false);

    MPI_Status status;
    int flag = 0;


    long long crashes = 0;

    while((!std::all_of(thread_done.begin(), thread_done.end(), [](bool b) { return b;})) || flag)  {
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
      if(flag) {
        int from = status.MPI_SOURCE;
        int tag = status.MPI_TAG;
        long long count = 0;
        int message = 0;
        if(tag == MESSAGE_COUNT) {
          MPI_Recv(&count, 1, MPI_LONG_LONG, from, tag, MPI_COMM_WORLD, &status);
          num_finished += count;
          print_bar(std::cout, (((float)num_sent) / n_inits));
        }
        if(tag == MESSAGE_STATUS) {
          MPI_Recv(&message, 1, MPI_INT, from, tag, MPI_COMM_WORLD, &status);
          if(message == STATUS_DONE) {
            //std::cout << "Process " << from << " done." << std::endl;
            thread_done[from - 1] = true;
          }
          else if(message == STATUS_REQUEST) {
            if(num_sent < n_inits && !interrupted) {
              message = STATUS_REQUEST;
              MPI_Send(&message, 1, MPI_INT, from, MESSAGE_STATUS, MPI_COMM_WORLD);
              MPI_Send(gens[num_sent].data(), num_each, MPI_INT, from, MESSAGE_DATA, MPI_COMM_WORLD);
              num_sent++;
            } else {
              message = STATUS_DONE;
              MPI_Send(&message, 1, MPI_INT, from, MESSAGE_STATUS, MPI_COMM_WORLD);
            }
          }
          else {
            std::cout << "Status message: " << message << std::endl;
          }
        }
        if(tag == MESSAGE_MISMATCH) {
          std::cout << "Mismatch found!" << std::endl;
          int mismatch[n_elements * 4];
          MPI_Recv(&mismatch, n_elements * 4, MPI_INT, from, tag, MPI_COMM_WORLD, &status);
          std::vector<int> mm;
          mm.resize(n_elements * 4);
          for(int i = 0; i < mm.size(); i++) {
            mm[i] = mismatch[i];
          }
          Configuration *mis = hist_from_ints(mm);

          write_file(&mis->history, "atomic-stack", "mismatches/" + std::to_string(mismatches) + ".hist");
          mismatches++;
          delete mis;
        } else if (tag == MESSAGE_CRASH) {
          std::cout << "Crash found!" << std::endl;
          std::vector<int> crash;
          crash.resize(n_elements * 4);
          MPI_Recv(&crash[0], n_elements * 4, MPI_INT, from, tag, MPI_COMM_WORLD, &status);

          Configuration *mis = hist_from_ints(crash);
          write_file(&mis->history, "atomic-stack", "crashes/" + std::to_string(crashes) + ".hist");
          crashes++;
        }
      }
    }
    delete[] data;
  } else {
    int curptr = 0;
    std::vector<int> init_v;

    init_v.resize(num_each, -1);
    bool fresh_init = true;
    long long counter = 0;
    while(fresh_init) {
      int msg = STATUS_REQUEST;
      MPI_Status status;

      MPI_Send(&msg, 1, MPI_INT, 0, MESSAGE_STATUS, MPI_COMM_WORLD);
      MPI_Recv(&msg, 1, MPI_INT, 0, MESSAGE_STATUS, MPI_COMM_WORLD, &status);
      if(msg == STATUS_DONE) {
        fresh_init = false;
      }
      else if (msg == STATUS_REQUEST) {
        // Acknowledgement
        MPI_Recv(init_v.data(), num_each, MPI_INT, 0, MESSAGE_DATA, MPI_COMM_WORLD, &status);


        auto generator = create_generator_prepended(n_elements, init_v, INT_MAX, max_thr);
        while(generator) {
          std::vector<int> int_h = generator();


          Configuration *simpl = hist_from_ints(int_h);
          MonitorConfig mc;
          mc.thread_count = simpl->num_threads;
          mc.type = simpl->type;

          Monitor *a = make_monitor(alg, mc);
          Monitor *b = make_monitor(cmp, mc);

          ComparisonResult res = try_history(simpl->history, a, b);

          if(res == Mismatch) {
            // Mismatch found, send to root!
            MPI_Send(int_h.data(), int_h.size(), MPI_INT, 0, MESSAGE_MISMATCH, MPI_COMM_WORLD);
          }
          if(res == MonitorCrash) {
            MPI_Send(int_h.data(), int_h.size(), MPI_INT, 0, MESSAGE_CRASH, MPI_COMM_WORLD);
          }

          if(res == MatchLin)
            linearizable++;
          if(res == MatchViol)
            violations++;

          delete a;
          delete b;
          delete simpl;
          counter++;
          if((counter % 10000) == 0) {
            MPI_Send(&counter, 1, MPI_LONG_LONG, 0, MESSAGE_COUNT, MPI_COMM_WORLD);
            counter = 0;
          }
        }
      }
    }
    delete[] init;
    int status = STATUS_DONE;

    MPI_Send(&counter, 1, MPI_LONG_LONG, 0, MESSAGE_COUNT, MPI_COMM_WORLD);
    MPI_Send(&status, 1, MPI_INT, 0, MESSAGE_STATUS, MPI_COMM_WORLD);
  }
  long long n_lin = 0;
  long long n_vio = 0;

  MPI_Reduce(&linearizable, &n_lin, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&violations, &n_vio, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  if(rank == 0) {
    if(interrupted) {
      std::cout << "Interrupted after " << num_sent << "/" << n_inits << " inits" << std::endl;
    }
    std::cout << "\nTotal:\t" << num_finished << "\n"
              << "Linearizable: " << n_lin << "\n"
              << "Violations: " << n_vio << "\n"
              << "Mismatches: " << mismatches << std::endl;
  }

  MPI_Finalize();
  return 0;
}
