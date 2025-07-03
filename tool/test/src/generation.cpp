#include "generator.hpp"
#include "io.h"

#define EXEC
// #define PRINT

Configuration *hist_from_ints(int count, int *data, ExhaustiveGenerator::HistoryType type) {
  std::vector<event_t> history;
  auto h = ExhaustiveGenerator::make_history(count, data, type);
  tid_t ts = 0;

  tid_t max_t = 0;

  //Need to change this: make a per-hisory ev_t -> event_t function
  std::transform(h.begin(), h.end(), std::back_inserter(history), [&ts, &max_t, &type](ExhaustiveGenerator::ev_t evt) {
    ts++;
    max_t = std::max(evt.th, max_t);
    // return event_t(evt.t, evt.th, val_t(evt.th, 0), ts);
    return ExhaustiveGenerator::getEventFromEv(evt, ts, type);
  });

  ADT adt = ADT::stack;
  for(int i = 0; i < count; i++) {
    if (data[i] == -1)
      adt = ADT::durable_stack;
  }

  // std::cout<<"ADT is "<<adt<<"\n";
  Configuration *conf = new Configuration(history, adt, history.size(), (adt == ADT::stack));
//   if(conf->needs_simpl)
//   {
//     Configuration *simpl = simplify(conf);
//     delete conf;

//     return simpl;
//   }
// //   else
//   {
//     return conf;
//   }

  return conf;
}


Configuration *hist_from_ints(int sz, std::vector<int> &int_h, ExhaustiveGenerator::HistoryType type) {
  return hist_from_ints(sz, int_h.data(), type);
}


int main() {

    ExhaustiveGenerator exGen;

    int size = 2;
    int crashes = 0;
    std::cout<<"Enter size: ";
    std::cin>>size;
    std::cout<<"Enter crashes: ";
    std::cin>>crashes;

    exGen.setHistoryType(ExhaustiveGenerator::UNKNOWN_AFTER);
    exGen.setNumCrashes(crashes);
    std::vector<std::vector<int>> inits;
    for(int i = 0; i < 8; i++) {
        inits = exGen.create_inits(size, i);
        std::cout << "min = " << i << " gives " << inits.size() << " histories" << std::endl;
    }

    // std::vector<char> init;
    // init.resize(size, 4);
    // auto hists = exGen.gen_histories(init);
    std::vector<int> pre;
    auto hists = exGen.create_generator_prepended(size,pre);

    long long count = 0;

    while(hists) {
        auto h = hists();
        count++;
#ifdef PRINT
        Configuration* simpl = hist_from_ints(size, h, ExhaustiveGenerator::UNKNOWN_AFTER);
        write_file(&simpl->history, "atomic-stack", "histories/test/hists/" + std::to_string(count) + ".hist");
        delete simpl;
#endif

#ifdef EXEC
        Configuration* simpl = hist_from_ints(size, h, ExhaustiveGenerator::UNKNOWN_AFTER);
        delete simpl;
#endif
    }

    // std::cout << "Generated " << count << " histories\n";

    long long iter_count = 0;

    for(auto it : inits) {
        auto hists = exGen.create_generator_prepended(size, it);
        while(hists) {
            std::vector<int> h = hists();
            iter_count++;
#ifdef PRINT
            Configuration* simpl = hist_from_ints(size, h, ExhaustiveGenerator::UNKNOWN_AFTER);
            write_file(&simpl->history, "unknown_after", "histories/test/inits/" + std::to_string(iter_count) + ".hist");
            delete simpl;
#endif
#ifdef EXEC
            Configuration* simpl = hist_from_ints(size, &h[0], ExhaustiveGenerator::UNKNOWN_AFTER);
            std::cout<< iter_count << "\n";
            delete simpl;
#endif

            
            
        }
    }


    std::cout << count << " hists of size " << size << ", with inits: " << iter_count << std::endl;

}
