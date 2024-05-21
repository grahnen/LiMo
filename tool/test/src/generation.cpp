#include "generator.hpp"

int main() {
    int size = 4;
    std::vector<std::vector<int>> inits;
    for(int i = 0; i < 8; i++) {
        inits = create_inits(size, i);
        std::cout << "min = " << i << " gives " << inits.size() << " histories" << std::endl;
    }

    std::vector<char> init;
    init.resize(size, 4);
    auto hists = gen_histories(init);

    long long count = 0;

    while(hists) {
        hists();
        count++;
    }


    long long iter_count = 0;

    for(auto it : inits) {
        auto hists = create_generator_prepended(size, it);
        while(hists) {
            hists();
            iter_count++;
        }
    }


    std::cout << count << " hists of size " << size << ", with inits: " << iter_count << std::endl;

}
