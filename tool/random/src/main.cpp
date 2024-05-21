#include <iostream>
#include <boost/program_options.hpp>
#include <mpi.h>
#include "algorithm.hpp"
#include <random>
#include "generator.hpp"
#include "execution.hpp"

using namespace boost::program_options;

using namespace std;


bool verbose;
Algorithm alg;
Algorithm cmp;
ADT adt;


int n_elements;
tid_t max_threads;
int num_histories;

bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
      ("verbose,v", value(&verbose)->default_value(false)->implicit_value(true), "Verbose")
      ("algorithm,a", value(&alg)->default_value(DefaultAlgorithm), "Algorithm")
      ("compare,c", value(&cmp)->default_value(Algorithm::cover), "Comparison Algorithm")
      ("threads,t", value(&max_threads)->default_value(MAX_THREADS))
      ("adt,d", value(&adt)->default_value(ADT::stack), "ADT to check")
      ("size", value(&n_elements)->required(), "Number of values")
      ("num_tests", value(&num_histories)->required(), "Number of tests");

    positional_options_description pos;
    pos.add("size", 1);
    pos.add("num_tests", 2);
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
  MPI_Init(&argc, &argv);
  int size, rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if(rank == 0) {
    if(!get_options(argc, argv)) {
      throw std::logic_error("Invalid arguments");
    }
  }

  //Broadcast args
  MPI_Bcast(&n_elements, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&num_histories, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&alg, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&adt, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&cmp, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&max_threads, 1, MPI_INT, 0, MPI_COMM_WORLD);
  //Randomize histories
  int num_self = num_histories / size + (rank < num_histories % size ? 1 : 0);


  std::random_device rd;
  std::mt19937_64 rng(rd());

  std::vector<int> my_mismatches;
  std::vector<int> my_crashes;
  int my_max_constr;
  tid_t my_max_conc;

  MonitorConfig mc;
  mc.type = adt;
  mc.thread_count = max_threads;

  for(int i = 0; i < num_self; i++) {
    std::vector<int> history = create_single(n_elements, max_threads, rng);
    Configuration *conf = hist_from_ints(history);
    conf->type = adt;
    my_max_conc = std::max(my_max_conc, conf->num_threads);
    mc.thread_count = conf->num_threads;
    mc.type = adt;
    std::stringstream os;
    Monitor *a = make_monitor(alg, mc);
    Monitor *b = make_monitor(cmp, mc);
    ComparisonResult cr = try_history(conf->history, a, b, verbose);
    if(cr == Mismatch){
      // Mismatch found, send to root!
      my_mismatches.insert(my_mismatches.end(), history.begin(), history.end());
    } else if (cr == MonitorCrash) {
      my_crashes.insert(my_crashes.end(), history.begin(), history.end());
    }
    delete a;
    delete b;
    delete conf;
  }
  int mmcount = my_mismatches.size();
  int mccount = my_crashes.size();

  int *mismatch_counts = (rank == 0) ? new int[size] : 0;
  int *crash_counts = (rank == 0) ? new int[size] : 0;
  int *mismatch_displs = (rank == 0) ? new int[size] : 0;
  int *crash_displs = (rank == 0) ? new int[size] : 0;

  int *mismatches = 0;
  int *crashes = 0;


  MPI_Gather(&mmcount, 1, MPI_INT, mismatch_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gather(&mccount, 1, MPI_INT, crash_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

  int mmsize = 0;
  int crsize = 0;
  if(rank == 0) {


    for(int i = 0; i < size; i++) {
      mismatch_displs[i] = mmsize;
      crash_displs[i] = crsize;
      mmsize += mismatch_counts[i];
      crsize += crash_counts[i];
    }
    mismatches = new int[mmsize];
    crashes = new int[crsize];
  }

  int max_constr_size;
  int max_conc_size;

  MPI_Gatherv(&my_mismatches[0], mmcount, MPI_INT, mismatches,mismatch_counts, mismatch_displs, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Gatherv(&my_crashes[0], mccount, MPI_INT, crashes,crash_counts, crash_displs, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Reduce(&my_max_constr, &max_constr_size, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&my_max_conc, &max_conc_size, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  if(rank == 0) {

    delete[] mismatch_counts;
    delete[] crash_counts;

    delete[] mismatch_displs;
    delete[] crash_displs;

    int num_mismatches = mmsize / ( 4 * n_elements);
    int num_crashes = crsize / ( 4 * n_elements);

    for(int i = 0; i < num_mismatches; i++) {
      Configuration *conf = hist_from_ints(n_elements, &mismatches[i * 4 * n_elements]);
      write_file(&conf->history, "atomic-stack", "mismatches/" + std::to_string(i) + ".hist");
    }

    for(int i = 0; i < num_crashes; i++) {
      Configuration *conf = hist_from_ints(n_elements, &crashes[i * 4 * n_elements]);
      write_file(&conf->history, "atomic-stack", "crashes/" + std::to_string(i) + ".hist");
    }

    delete[] mismatches;
    delete[] crashes;


    std::cout << "Finished with " << num_crashes << " crashes and " << num_mismatches << " mismatches." << std::endl;
    std::cout << "Maximum constraint: " << max_constr_size << std::endl;
    std::cout << "Maximum concurrency: " << max_conc_size << std::endl;
  }

  //Finish!
  MPI_Finalize();
  return 0;
}
