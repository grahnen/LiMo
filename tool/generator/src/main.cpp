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
int max_crashes;
std::string filename;

bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
      ("threads,t", value(&max_threads)->default_value(MAX_THREADS))
      ("adt,d", value(&adt)->required(), "ADT to check")
      ("size", value(&n_elements)->required(), "Number of values")
      ("crashes,c", value(&max_crashes)->default_value(0), "Number of crashes")
      ("num_tests", value(&num_histories)->required(), "Number of tests")
      ("filename,o", value(&filename)->required(), "Output file name");

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
  if(!get_options(argc, argv)) {
      throw std::logic_error("Invalid arguments");
    }
  ExhaustiveGenerator exGen;
  ExhaustiveGenerator::HistoryType type = ExhaustiveGenerator::NORMAL;
  switch (adt)
  {
  case ADT::unknown_after:
      type = ExhaustiveGenerator::UNKNOWN_AFTER; 
      break;
  case ADT::registers:
      type = ExhaustiveGenerator::REGISTER; 
      break;
  default:
      // std::cout << "no!!!" << adt <<"-"<< int(ADT::unknown_after) <<"\n";
      break;
  }
  exGen.setHistoryType(type);
  exGen.setNumCrashes(max_crashes);

  std::random_device rd;
  std::mt19937_64 rng(rd());
  
  auto hist_ints = exGen.create_single(n_elements, max_threads, rng);

  Configuration *conf = hist_from_ints(n_elements, hist_ints, type);
  conf->type = adt;

  write_file(&(conf->history), ext2str(adt) , filename);
}
