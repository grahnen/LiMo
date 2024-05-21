#include <vector>
#include <string>
#include "io.h"
#include <cassert>

const std::vector<const char *> pops {
  "[1] call pop",
  "[1] call pop()"
};

const std::vector<const char *> pushes {
  "[1] call push 2",
  "[1] call push(2)"
};

int main() {
  std::map<tid_t, int> counts;
  std::map<int, val_t> value_map;
  std::map<std::string, int> varmap;

  std::optional<event_t> evt = {};
  for(auto it : pushes) {
    counts.clear();
    value_map.clear();
    bool needs_simpl = false;
    std::optional<event_t> curr = get_event(it, counts, value_map, varmap, &needs_simpl);
    std::cout << curr.value() << std::endl;
    if(evt.has_value()) {
      assert(curr.has_value());
      curr->timestamp = 3;
      assert(evt.value() == curr.value());
    } else {
      if(curr.has_value())
        curr->timestamp = 3;
      evt = curr;
    }
  }

}
