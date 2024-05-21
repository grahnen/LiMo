#include "builder.hpp"
#include "monitor.hpp"
#include "generator.hpp"
#include "io.h"
#include "convert.h"

int thread_count(std::vector<event_t> &hist) {
    auto max_t_it = std::max_element(hist.begin(), hist.end(), [](event_t a, event_t b) {
        return a.thread < b.thread;
    });
    if(max_t_it == hist.end()) {

        throw std::logic_error("Empty history?");
    }
    return max_t_it->thread + 1;
}

int get_threads(int num_elems) {
    evec hist = generate_lin_hist(num_elems);

    Configuration *conf = new Configuration(hist, stack, thread_count(hist));
    Configuration *tmp = simplify(conf);

    delete conf;
    conf = tmp;
    tmp = NULL;

    int max_t = thread_count(conf->history);
    delete conf;
    return max_t;
}

int main(int argc, char *argv[]) {
    int reps = 100;
    for(int i = 100000; i < 100001; i++) {
        float tot = 0;
        for(int r = 0; r < reps; r++) {
            tot += get_threads(i);
        }

        std::cout << i << ":\t" << (tot / reps) << std::endl;
    }
}
