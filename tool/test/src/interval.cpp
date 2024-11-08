#include "interval.h"
#include <cassert>
#include <iostream>

int main() {
  Interval I1;
  Interval I2;
  assert(I1 == I2);

  I1 = I1 + AtomicInterval::complete();
  assert(I1 != I2);

  I2 = I2 + AtomicInterval::complete();

  assert(I1 == I2);

  I1 = Interval::open(0, 2);
  I2 = Interval::closed(1, 5);
  Interval I3 = I1 * I2;
  Interval I4({ AtomicInterval(false, 1, 2, true) });
  assert(I3 == I4);
  assert(I3 + I4 == I4);
  assert(I3 + I3 == I3);
  Interval I5({ AtomicInterval(true, 0, 5, false) });
  std::cout << (I1 + I2) << " == " << I5 << std::endl;
  assert(I1 + I1 == I1);
  assert(I2 + I2 == I2);
  assert(I5 + I5 == I5);
  std::cout << "I1: " << I1 << std::endl << "I2: " <<  I2 << std::endl;
  std::cout << "I1 + I2: " << (I1 + I2) << std::endl;
  std::cout << "I1 * I2: " << (I1 * I2) << std::endl;

  assert(I1 + I2 == I5);
  std::cout << I3 << " == " << I4 << std::endl;


  Interval a = Interval::open(5,10);
  Interval b = Interval::closed(7, 15);
  Interval c = Interval::open(6, 9);
  Interval d = Interval::closed(3, 9);

  assert(a + c == a);
  assert(a * c == c);
  assert(b + d == Interval::closed(3, 15));
  assert(b * d == Interval::closed(7, 9));

  assert(a + b == Interval::openclosed(5, 15));
  assert(a * b == Interval::closedopen(7, 10));

  std::cout << (a + d) << std::endl;
  assert(a + d == Interval::closedopen(3, 10));
  assert(a * d == Interval::openclosed(5, 9));

  AtomicInterval ai = AtomicInterval::closed(3, 5);
  AtomicInterval bi = AtomicInterval::closed(4, 6);

  AtomicInterval ci = AtomicInterval::closed(4, 5);

  std::cout << ai << bi << ci << std::endl;
  assert(ai * bi == ci);


  std::cout << "New test: " << std::endl;

  AtomicInterval x = AtomicInterval::closed(3, 6);
  AtomicInterval y = AtomicInterval::closed(7, 8);
  AtomicInterval z = AtomicInterval::closed(5, 11);

  std::cout << (x + y) + z << std::endl;
  assert(((x + y) + z) == x + z);

  std::cout << "Complement" << std::endl;
  Interval q(std::vector {AtomicInterval::closed(1,4), AtomicInterval::closed(6, 11)});

  std::cout << q << std::endl;
  std::cout << q.complement() << std::endl;
}
