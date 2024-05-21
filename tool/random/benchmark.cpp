#include <boost/program_options.hpp>
#include <iostream>
#include <mpi.h>
#include "algorithm.hpp"
#include "execution.hpp"
#include "suite.hpp"
#include "generator.hpp"
#include "io.h"

using namespace boost::program_options;

MPI_Datatype config_datatype() {
    MPI_Datatype type;

    int blocksCount = 3;
    int blocksLength[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint offsets[3] = {offsetof(config, threads), offsetof(config, pushes), offsetof(config, pops)};
    MPI_Type_create_struct(blocksCount, blocksLength, offsets, types, &type);

    MPI_Type_commit(&type);
    return type;
}



Algorithm algorithm;
std::string filename;
std::string suite;
bool verbose = false;
int reps;
int increment;


bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
      ("verbose,v", value(&verbose)->default_value(false)->implicit_value(true), "Verbose")
      ("suite,s", value(&suite)->default_value("")->implicit_value("suite.txt"))
      ("increment,i", value(&increment)->default_value(0)->implicit_value(1))
      ("algorithm,a", value(&algorithm)->default_value(Algorithm::interval), "Algorithm (only for benchmark)")
      ("repetitions,r", value(&reps)->default_value(1)->implicit_value(10), "Number of repetitions")
      ("output", value(&filename)->required(), "Output file");

    positional_options_description pos;
    pos.add("output", 1);
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
    int n_configs = 0;

    if(rank == 0) {
        if(!get_options(argc, argv)) {
            throw std::logic_error("Invalid arguments");
        }
        confs = load_suite(suite, increment);
        for(auto c : confs) {
            assert(c.pushes == c.pops);
        }
        n_configs = confs.size();

    }

    //MPI_Bcast(&algorithm, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //MPI_Bcast(&reps, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //MPI_Bcast(&n_configs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(rank != 0) {
        confs.resize(n_configs);
    }

    //MPI_Bcast(confs.data(), n_configs, config_datatype(), 0, MPI_COMM_WORLD);

    int num_iters_per = reps / size;
    int total_size = confs.end()->pushes;
    std::random_device dev;
    std::mt19937_64 rand(dev());
    for(auto cfg : confs) {
        int *indices = new int[total_size];
        std::vector<char> initial_state;
        initial_state.resize(cfg.pushes);

        gen_random_indices(initial_state, indices, rand, cfg.threads);

        Configuration *conf = hist_from_ints(cfg.pushes, indices);
        assert(conf->threads <= cfg.threads);

    }


    MPI_Finalize();
    return 0;
}
