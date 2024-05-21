#include "logic.hpp"
#include "relation.h"
#include <cassert>

int main() {
  Relation
    ab(RelVal::Above),
    be(RelVal::Below),
    bf(RelVal::Before),
    af(RelVal::After);

  Value q(0, AtomicInterval::closed(1,3), AtomicInterval::closed(7, 8));
  Value w(1, AtomicInterval::closed(2,4), AtomicInterval::closed(9,10));
  std::cout << compare(q,w) << std::endl;


  Value a(0, AtomicInterval::closed(1, 3), AtomicInterval::closed(6, 9));
  Value b(1, AtomicInterval::closed(2, 5), AtomicInterval::closed(8, 11));
  Value c(2, AtomicInterval::closed(4, 7), AtomicInterval::closed(12, 13));
  Value c2(2, AtomicInterval::closed(4,7), AtomicInterval::closed(10, 13));
  std::cout << "VDHK2-noc" << std::endl;
  std::cout << "ab: " << compare(a, b) << std::endl;
  assert(compare(a,b) == (ab | be));
  std::cout << "ac: " << compare(a, c) << std::endl;
  assert(compare(a,c) == bf);
  std::cout << "bc: " << compare(b,c) << std::endl;
  assert(compare(b,c) == ab);
}
