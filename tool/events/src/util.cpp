#include "util.h"

template <typename P, typename Q>
std::optional<Q> transform(std::optional<P> o, std::function<P(Q)> f) {
  if(o.has_value())
    return f(o);
  return {};
}
