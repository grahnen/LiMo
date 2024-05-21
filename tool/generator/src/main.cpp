#include <iostream>

#include "algorithm.hpp"
#include <random>
#include "generator.hpp"
#include "execution.hpp"


#include <boost/program_options.hpp>

using namespace boost::program_options;

ADT adt;

int n_elements;
tid_t max_threads;
int num_histories;

bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
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

}
