#include "logic.hpp"
#include <cassert>


int main() {

  BP a = std::make_shared<Bound>(AtomicInterval::closed(4, 8), AtomicInterval::closed(11, POSINF));

  BP b = std::make_shared<Bound>(AtomicInterval::closed(7, 8), AtomicInterval::closed(11, POSINF));
  BP c = std::make_shared<Bound>(AtomicInterval::closed(4, 8), AtomicInterval::closed(11, 12));


  TP join = std::make_shared<Conj>( std::vector<TP> {
    a,
    std::make_shared<Disj>(std::vector<TP> {
        b,
        c
      })

  });
  std::cout << join << std::endl;
  std::cout << join->simpl() << std::endl;
  return 0;
}
