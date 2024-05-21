#include <iostream>
#include "event.h"
#include "algorithm.hpp"
#include "io.h"
#include "exception.h"
#include "convert.h"

#include <chrono>

using clk = std::chrono::high_resolution_clock;
using dur = std::chrono::high_resolution_clock::duration;

#include <boost/program_options.hpp>

using namespace boost::program_options;
using namespace std::chrono_literals;


std::string filename;
std::string dot_dir;
bool verbose;
Algorithm algorithm;

bool get_options(int argc, char *argv[]) {
  try {
    options_description desc { "Options" };
    desc.add_options()
      ("help,h", "Help")
      ("verbose,v", value(&verbose)->default_value(false)->implicit_value(true), "Verbose")
      ("algorithm,a", value(&algorithm)->default_value(segment), "Algorithm")
      ("dotdir,d", value(&dot_dir)->default_value("dots"), "Dot graph output directory")
      ("input", value(&filename)->required(), "Input File");

    positional_options_description pos;
    pos.add("input", 1);
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

void print_data(Monitor *m, Configuration *c, std::optional<dur> read_dur, std::optional<dur> simpl_dur, std::optional<dur> lin_dur) {
  std::cout << "ADT: " << c->type << std::endl;
  std::cout << "Threads: " << c->num_threads << "\tEvents: " << c->history.size() << std::endl;
  if(read_dur.has_value()) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(read_dur.value()).count();
    std::cout << "Reading: " << (ms / 1000.0) << "\t";
  }
  if(simpl_dur.has_value()) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(simpl_dur.value()).count();
    std::cout << "Simplification: " << (ms / 1000.0) << "\t";
  }
  if(lin_dur.has_value()) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(lin_dur.value()).count();
    std::cout << "Linearization: " << (ms / 1000.0);
  }
  if(read_dur.has_value() || simpl_dur.has_value() || lin_dur.has_value())
    std::cout << std::endl;


  if(m != nullptr)
    std::cout << "Max conc: " << m->max_conc() << " Avg conc: " << m->avg_conc() << std::endl;
}


int main(int argc, char *argv[]) {
  
  if (!get_options(argc, argv)) {
    return -1;
  }
  auto tr = clk::now();
  Configuration *c = read_log(filename);
  auto tr2 = clk::now();
  dur read_dur = tr2 - tr;

  dur simpl_dur, lin_dur;

  try {
    if(c->needs_simpl) {
      auto t = clk::now();
      Configuration *nc = simplify(c);
      auto t2 = clk::now();
      simpl_dur = t2 - t;
      delete c;
      c = nc;
    }
  } catch(std::logic_error &e) {
    std::cout << "Logic Error: " << e.what() << std::endl;
    print_data(nullptr, c, read_dur, {},{});
    if(c)
      delete c;
    return 0;
  } catch(Violation &e) {
    std::cout << "Violation: " << e.what() << std::endl;
    print_data(nullptr, c, read_dur, {}, {});
    if(c)
        delete c;
    return 0;
  }
  MonitorConfig mc;
  mc.type = c->type;
  mc.dot_dir = dot_dir;
  mc.thread_count = c->num_threads;

  Monitor *m = make_monitor(algorithm, mc);

  if (m == nullptr) {
    std::cout << "Invalid algorithm choice" << std::endl;
    delete c;
    exit(1);
  }

  m->set_verbose(verbose);
  auto t = clk::now();
  try {

    for (event_t e : c->history) {
      m->add_event(e);
    }
    auto t2 = clk::now();
    std::cout << "Time before post: " << (t2 - t) << std::endl;
    m->do_linearization();
    t2 = clk::now();
    lin_dur = t2 - t;
    print_data(m, c, read_dur, simpl_dur, lin_dur);
    std::cout << "OK" << std::endl;
  } catch (Violation &e) {
    auto t2 = clk::now();
    lin_dur = t2 - t;
    print_data(m, c, read_dur, simpl_dur, lin_dur);
    std::cout << "Violation: " << e.what();
  } catch (std::logic_error &e) {
    print_data(m, c, read_dur, simpl_dur, lin_dur);
    std::cout << "Logic error: " << e.what() << std::endl;
  } catch (std::exception &e) {
    print_data(m, c, read_dur, simpl_dur, lin_dur);
    std::cout << "Runtime exception: " << e.what() << std::endl;;
    delete m;
    delete c;
    return 0;
  }


  delete m;
  delete c;

  return 0;
}
