#include <boost/program_options.hpp>
#include <iostream>
#include <mpi.h>
#include "algorithm.hpp"
#include "execution.hpp"
#include "suite.hpp"
#include "generator.hpp"
#include "io.h"
#include "results.hpp"
#include "builder.hpp"

#define KEEP_ONLY_LIN 1

using namespace boost::program_options;

void config_datatype(MPI_Datatype *type) {
    int blocksCount = 3;
    int blocksLength[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint offsets[3] = {offsetof(config, threads), offsetof(config, values) };
    MPI_Type_create_struct(blocksCount, blocksLength, offsets, types, type);

    MPI_Type_commit(type);
}



Algorithm algorithm;
std::string filename;
std::string suite;
bool verbose = false;
int reps = 0;
int increment = 0;
int n_configs = 0;


#if DEBUGGING
int iterations = 0;
int largest_conc = 0;
#endif

bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
      ("verbose,v", value(&verbose)->default_value(false)->implicit_value(true), "Verbose")
      ("increment,i", value(&increment)->default_value(0)->implicit_value(1))
      ("algorithm,a", value(&algorithm)->default_value(Algorithm::interval), "Algorithm (only for benchmark)")
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

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::vector<config> confs;

    MPI_Datatype conf_type;
    config_datatype(&conf_type);

    if(rank == 0) {
        if(!get_options(argc, argv)) {
            throw std::logic_error("Invalid arguments");
        }
        confs = load_suite(suite, increment);
        n_configs = confs.size();

        if(reps % size != 0) {
            reps = ((reps / size) * size);
            std::cout << "repetitions does not divide size. Using reps = " << reps << std::endl;
        }

        if(verbose)
            std::cout << "Verbose mode enabled" << std::endl;
    }

    MPI_Bcast(&algorithm, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(rank == 0 && verbose)
        std::cout << "Broadcasted algorithm" << std::endl;
    MPI_Bcast(&reps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(rank == 0 && verbose)
        std::cout << "Broadcasted repetitions" << std::endl;

    if(rank == 0 && verbose)
        std::cout << "Config count: " << n_configs << std::endl;

    std::cout << "Process " << rank << " at barrier" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
    std::cout << "Process " << rank << " ready" << std::endl;

    MPI_Bcast(&n_configs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    std::cout << "Process " << rank << " done" << std::endl;

    if(rank == 0 && verbose)
        std::cout << "Broadcasted configuration count" << std::endl;


    if(rank != 0) {
        confs.resize(n_configs);
    }

    MPI_Bcast(confs.data(), n_configs, conf_type, 0, MPI_COMM_WORLD);
    if(rank == 0 && verbose)
        std::cout << "Broadcasted configuration data" << std::endl;

    int num_iters_per = reps / size;
    int total_size = confs.rbegin()->values;
    std::random_device dev;
    std::mt19937_64 rand(dev());
    int *indices = new int[total_size * 4];
    double *benchmark_results = new double[n_configs * num_iters_per];


    std::random_device rd;
    std::mt19937_64 rng(rd());

    NullStream ns;
    for(int c = 0; c < n_configs; c++) {
        auto cfg = confs[c];
        if(rank == 0) {
            std::cout << "Starting config " << cfg << std::endl;
        }

        for(int i = 0; i < num_iters_per; i++) {
            Monitor *m = 0;
            Configuration *conf = 0;

            #if KEEP_ONLY_LIN

            std::vector<int> history = create_single(cfg.values, cfg.threads, rng);

            conf = hist_from_ints(history);

            MonitorConfig mc;

            m = make_monitor(algorithm, mc);

            double start = MPI_Wtime();
            bool res = try_hist(m, conf->history);
            double stop = MPI_Wtime();

            #else
// Old stuff
            gen_random_indices(initial_state, indices, rand, cfg.threads);


                conf = hist_from_ints(cfg.pushes, indices);
                assert(conf->num_threads <= cfg.threads);

                m = make_monitor(algorithm, mc);

                double start = MPI_Wtime();
                bool res = try_hist(m, conf->history);
                double stop = MPI_Wtime();


                i--;
                delete m;
                delete conf;
                initial_state.clear();
                continue;
            }
            #endif

            benchmark_results[c * num_iters_per + i] = stop-start;

            delete m;
            delete conf;
        }

    }
    delete[] indices;


    std::map<config, std::vector<double>> main_results;


    for(int c = 0; c < n_configs; c++) {
        if(rank == 0) {
            main_results[confs[c]] = std::vector<double>();
            main_results[confs[c]].resize(num_iters_per * size);
        }
        MPI_Gather(&benchmark_results[c * num_iters_per], num_iters_per, MPI_DOUBLE, &main_results[confs[c]][0], num_iters_per, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }
#if DEBUGGING
int lconc = 0;
int liters = 0;
int lconstr = 0;
MPI_Reduce(&largest_conc, &lconc, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
MPI_Reduce(&iterations, &liters, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

if(rank == 0) {
    std::cout << "Largest constraint: " << lconstr << std::endl;
}

#endif

    if(rank == 0) {
        write_results(main_results, filename);
    }
    MPI_Type_free(&conf_type);
    MPI_Finalize();
    return 0;
}
