#ifndef RESULTS_H_
#define RESULTS_H_
#include "suite.hpp"
#include <fstream>

void write_results(std::map<config, std::vector<double>> durations, std::string filename) {
  std::fstream f;
  bool first = true;
  f.open(filename, std::ios::out);
  f << "{ " << std::endl;
  for(auto it : durations) {
    if(it.second.size() == 0)
      continue;
    if(first)
      first = false;
    else
      f << ",";

    f << "  \"" << it.first << "\":" << std::endl;
    f << "  [  " << it.second[0] << std::endl;

    for(int i = 1; i < it.second.size(); i++) {
      f << "  ,  " << it.second[i] << std::endl;
    }
    f << "]" << std::endl;
  }
  f << "}" << std::endl;
}


#endif // RESULTS_H_
