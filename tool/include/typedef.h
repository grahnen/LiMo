#ifndef TYPEDEF_H
#define TYPEDEF_H
#include <optional>
#include <ostream>
#include <climits>
#include "util.h"
#include <functional>
#include <iostream>

//Make this as big as we can to make sure we *never* overflow in a reasonable execution.
using timestamp_t = unsigned long long;
#define NEGINF 0
#define POSINF ULLONG_MAX
using tid_t = timestamp_t;
#define MAX_THREADS ULLONG_MAX


#define MINIDX LLONG_MIN
#define MAXIDX LLONG_MAX
using index_t = long long;


struct val_t {
  char input_val[10];
  tid_t thread;
  index_t idx;

  val_t() : val_t(-1) {}
  val_t(int i) : val_t(-1, i) {}
  val_t(tid_t t, int i) : val_t("?", t, i) {  }
  val_t(const char *iv, tid_t t, index_t i) : thread(t), idx(i) {
    for(int i = 0; i < 10; i++) {
      input_val[i] = iv[i];
      if(iv[i] == '\0')
        break;
    }
  }

  val_t(const val_t &o) = default;
  val_t &operator=(const val_t &o) = default;
  ~val_t() = default;

  inline bool operator==(const val_t &o) const {
    return thread == o.thread && idx == o.idx;
  }

  inline bool operator<(const val_t &o) const {
    if (thread == o.thread)
      return idx < o.idx;

    return thread < o.thread;

  }

  inline friend std::ostream &operator<<(std::ostream &os, const val_t &v) {
    if (v.thread == -1) {
      return os << v.idx;
    }
    return os << v.input_val << "<" << v.thread << "," << v.idx << ">";
  }
  friend std::string to_string(val_t &v) {
    return ext2str(v);
  }

  val_t &operator=(const int b) {
    thread = -1;
    idx = b;
    input_val[0] = '?';
    input_val[1] = '\0';
    return *this;
  }
};
using optval_t = std::optional<val_t>;
using res_t = optval_t;

template<typename T>
inline constexpr std::ostream& operator<<(std::ostream &os, const std::optional<T> &o) {
  return os << (o.has_value() ? ext2str(o.value()) : "ø");
}


enum ADT : int {
unknown = 0,
stack = 1 << 1,
queue = 1 << 2,
set = 1 << 3,
};

inline constexpr ADT operator|(const ADT &a, const ADT &b) {
  return (ADT) (((int) a) | (int) b);
}
inline constexpr ADT operator^(const ADT &a, const ADT &b) {
  return (ADT) (((int) a) ^ (int) b);
}

inline constexpr ADT operator&(const ADT &a, const ADT &b) {
  return (ADT) (((int) a) & (int) b);
}

inline constexpr ADT operator^=(ADT &a, const ADT &b) {
  return a = (a ^ b);
}

inline std::istream& operator>>(std::istream& in, ADT &adt) {
  std::string token;
  in >> token;
  if(token == "stack")
    adt = stack;
  else if (token == "queue")
    adt = queue;
  else if (token == "set")
    adt = set;
  else
    in.setstate(std::ios_base::failbit);

  return in;
}


inline constexpr std::ostream &operator<<(std::ostream &os, const ADT &a) {
  ADT c = a;
  if(c & stack) {
    os << "stack";
    c ^= stack;
    if (c > 0) {
      os << " | ";
    }
  }

  if(c & queue) {
    os << "queue";
    c ^= queue;
    if (c > 0) {
      os << " | ";
    }
  }
  if(c & set) {
    os << "set";
    c ^= set;
    if(c > 0) {
      os << " | ";
    }
  }
  return os;
}

#endif
