#ifndef UTIL_H
#define UTIL_H
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <optional>
#include <vector>
#include <functional>

template<typename T>
std::string ext2str(const T &a) {
  std::stringstream ss;
  ss << a;
  return ss.str();
}

template<typename T>
struct LinearizationResult {
  std::optional<std::string> viol;
  std::vector<T> remaining;

  bool violation() const { return viol.has_value(); }

  const char *error() const {
    if (viol.has_value()) {
      return viol.value().c_str();
    }
    return "OK";
  }

  inline LinearizationResult() : viol({}), remaining() {}
  inline LinearizationResult(std::string v) : viol(v), remaining() {}
  inline LinearizationResult(std::vector<T> oa)
      : viol({}), remaining(oa.begin(), oa.end()) {}

  inline LinearizationResult operator+(const LinearizationResult &o) const {
    if (viol.has_value())
      return LinearizationResult<T>(viol.value());

    if (o.viol.has_value())
      return LinearizationResult<T>(o.viol.value());

    std::vector<T> rm;
    rm.resize(o.remaining.size() + remaining.size());
    for (int i = 0; i < o.remaining.size(); i++) {
      rm[i] = o.remaining[i];
    }
    for (int i = 0; i < remaining.size(); i++) {
      rm[o.remaining.size() + i] = remaining[i];
    }
    return LinearizationResult(rm);
  }
};

enum Last {
  PopEmpty,
  Push,
  PushPop,
  Epsilon
};

// Array types (easy change)
template<typename T>
using Array = std::map<int,T>;
//using Array = TArray<T>;

// Set type (easy change)
template<typename T>
using Set = std::set<T>;

// String-to-(optional int)
inline std::optional<int> try_stoi(std::string s) {
  try {
    int i = std::stoi(s);
    return i;
  }
  catch (const std::exception &e) {
    return std::nullopt;
  }
}

template<typename P, typename Q>
std::optional<Q> transform(std::optional<P> o, std::function<P(Q)> f);
inline std::ostream &operator<<(std::ostream &os, std::optional<int> i) {
  if(i.has_value())
    return os << i.value();
  return os << "{}";
}

class NullStream : public std::ostream {
public:
    inline NullStream() : std::ostream(nullptr) {}
    inline NullStream(const NullStream &) : std::ostream(nullptr) {}
};

template<class T>
inline const NullStream &operator<<(NullStream &&os, const T &value) {
  return os;
}

inline void writefile(std::string filename, std::string contents) {
  std::ofstream f;
  f.open(filename);
  f << contents;
  f.close();
}


#endif

