#include <iostream>
#include <boost/program_options.hpp>
#include "io.h"

#include "builder.hpp"
#include "convert.h"
using namespace boost::program_options;

int threads;
int n_ops;
bool per_thread;


int ops_per_thread;
int total_ops;

bool get_options(int argc, char *argv[]) {
    try {
        options_description desc { "Options" };
        desc.add_options()
            ("threads", value(&threads)->required())
            ("operations,n", value(&n_ops)->required())
            ("per_thread,p", value(&per_thread)->default_value(false)->implicit_value(true));

        positional_options_description pos;
        pos.add("threads", 1);
        pos.add("operations", 2);
        variables_map vm;
        store(command_line_parser(argc, argv).positional(pos).options(desc).run(), vm);
        notify(vm);
    } catch (const boost::program_options::error &ex) {
        std::cerr << ex.what() << std::endl;
        return false;
    }

    return true;
}


bool fetch_parameters(int argc, char *argv[]) {
    if(!get_options(argc, argv))
        return false;

    if(per_thread) {
        ops_per_thread = n_ops;
        total_ops = n_ops * threads;
    }
    else {
        ops_per_thread = n_ops / threads;
        total_ops = ops_per_thread * threads;
        if(total_ops != n_ops)
            std::cout << "Rounding error\t" << n_ops - total_ops << std::endl;
    }

    std::cout << "Threads\t\t" << threads << std::endl;
    std::cout << "Ops/thread\t" << ops_per_thread << std::endl;
    std::cout << "Total ops\t" << total_ops << std::endl;

    return true;
}

int main(int argc, char *argv[]) {
    if(!fetch_parameters(argc, argv))
        return -1;


    std::vector<event_t> hist = generate_lin_hist(total_ops);
    Configuration *c = new Configuration(hist, stack, total_ops * threads, true);

    Configuration *n = simplify(c, true);
    std::cout << "simplified has " << n->num_threads << " threads" << std::endl;


    

    return 0;
}
