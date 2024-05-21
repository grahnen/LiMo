#ifndef SUITE_H_
#define SUITE_H_
#include <iostream>
#include <vector>
#include <regex>
#include <fstream>
#include <cstring>
struct config {
  int threads;
  int values;

  config(int t, int p ) : threads(t), values(p) {}
  config() : config(0,0) {}
  bool operator<(const config &o) const {
    if(threads != o.threads)
      return threads < o.threads;
    return values < o.values;
  }
};

std::ostream &operator<<(std::ostream &os, const config &c) {
  return os << c.threads << "-" << c.values;
}

std::regex range_regex("(?:\\[(\\d+):(\\d+)(?::(\\d+))?\\])|(\\d+)");
std::regex step_regex("([^ ]*) ([^ ]*)");

std::vector<int> get_idx(std::string s) {
  std::cmatch m;
  std::vector<int> v;
  if(std::regex_match(s.c_str(), m, range_regex)) {
    if(m[4].compare("") != 0) {
      v.push_back(std::stoi(m[4]));
    } else {
      int min, max, step;
      min = std::stoi(m[1]);
      max = std::stoi(m[2]);
      step = (m[3].compare("") == 0 ? 1 : std::stoi(m[3]));

      for(int i = min; i <= max; i+= step) {
        v.push_back(i);
      }
    }
  }

  return v;
}


std::vector<config> load_suite(std::string suite, int increment) {
  std::vector<config> configs;

  std::fstream file;
  file.open(suite, std::ios::in);

  if(!file.is_open())
    throw std::runtime_error("Cannot open file " + suite);

  char line[100];
  std::cmatch cm;

  //Get data type
  while (file.getline(line, 100)) {
    if (std::regex_match(line, cm, step_regex)) {
      std::cmatch m;
      std::vector<int> thrs = get_idx(cm[1]);
      std::vector<int> items = get_idx(cm[2]);
      for(int t = 0; t < thrs.size(); t++) {
        for(int i = 0; i < items.size(); i++) {
          configs.push_back(config(thrs[t], items[i]));
        }
      }
    }
  }
  file.close();
  return configs;
}



#endif // SUITE_H_
