#ifndef GENERATOR_H_
#define GENERATOR_H_
#include "event.h"
#include<optional>
#include <cassert>
#include <coroutine>
#include <ranges>
#include <random>
#include <algorithm>
/* Shamelessly stolen from cppreference */
template<typename T>
struct Generator {
   // The class name 'Generator' is our choice and it is not required for coroutine magic.
   // Compiler recognizes coroutine by the presence of 'co_yield' keyword.
   // You can use name 'MyGenerator' (or any other name) instead as long as you include
   // nested struct promise_type with 'MyGenerator get_return_object()' method.
   // Note: You need to adjust class constructor/destructor names too when choosing to
   // rename class.

  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  struct promise_type { // required
    T value_;
    std::exception_ptr exception_;

    Generator get_return_object() {
      return Generator(handle_type::from_promise(*this));
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception_ = std::current_exception(); } // saving
                                                                          // exception

    template<std::convertible_to<T> From> // C++20 concept
    std::suspend_always yield_value(From &&from) {
      value_ = std::forward<From>(from); // caching the result in promise
      return {};
    }
    void return_void() {}
  };

  handle_type h_;

  Generator(handle_type h) : h_(h) {}
  ~Generator() { h_.destroy(); }
  explicit operator bool() {
    fill(); // The only way to reliably find out whether or not we finished coroutine,
            // whether or not there is going to be a next value generated (co_yield) in
            // coroutine via C++ getter (operator () below) is to execute/resume coroutine
            // until the next co_yield point (or let it fall off end).
            // Then we store/cache result in promise to allow getter (operator() below to
            // grab it without executing coroutine).
    return !h_.done();
  }
  T operator()() {
    fill();
    full_ = false; // we are going to move out previously cached
                   // result to make promise empty again
    return std::move(h_.promise().value_);
  }

private:
  bool full_ = false;

  void fill() {
    if (!full_) {
      h_();
      if (h_.promise().exception_)
        std::rethrow_exception(h_.promise().exception_);
        // propagate coroutine exception in called context

      full_ = true;
    }
  }
};

struct ev_t {
  EType t;
  tid_t th;
  inline ev_t(EType t, tid_t th) : t(t), th(th) {}
};

inline ev_t evt(int i, char state) {
  switch(state) {
    case 4:
      return ev_t(Epush, i);
    case 3:
      return ev_t(Ereturn, i);
    case 2:
      return ev_t(Epop, i);
    case 1:
      return ev_t(Ereturn, i);
  }
  // Error
  return ev_t(Enil, i);
}

// void gen_random_indices(std::vector<char> &state, int *root, std::mt19937_64 &rand, tid_t max_threads = MAX_THREADS) {
//   int curr = 0;
//   bool done = false;
//   std::uniform_int_distribution<std::mt19937_64::result_type> dist(0, state.size() - 1);
//   while(!done) {
//     int concurrency = std::count_if(state.begin(), state.end(), [](int i ) {
//       return i == 1 || i == 3;
//     });

//     int i = dist(rand);
//     while(state[i] == 0 || ((state[i] == 2 || state[i] == 4) && concurrency >= max_threads)) {
//       i = dist(rand);
//     }

//     state[i]--;
//     root[curr] = i;

//     curr++;

//     done = std::all_of(state.begin(), state.end(), [](int i) {
//       return i == 0;
//     });
//   }




// }

// inline Generator<std::vector<int>> gen_indices(std::vector<char> state, int num_to_fix = INT_MIN, std::vector<int> prepend = std::vector<int>(), tid_t max_threads = MAX_THREADS) {
//   if(num_to_fix == 0) {
//     co_yield prepend;
//     co_return;
//   }
//   bool added_already = false;
//     if(std::all_of(state.begin(), state.end(), [](int i) {
//       return i == 0;
//     })) {

//       co_yield prepend;

//       co_return;
//     }
//     // Current num threads:
//     int concurrency = std::count_if(state.begin(), state.end(), [](int i ) {
//       return i == 1 || i == 3;
//     });

//     for(int i = 0; i < state.size(); i++) {
//       int rem = state[i];
//       if(rem > 0) {
//         if(rem == 4) {
//           if(added_already)
//             continue;
//           else
//             added_already = true;
//         }
//         if((rem == 2 || rem == 4) && concurrency >= max_threads) {
//           // Adding this event would exceed max_threads, so we don't.
//           continue;
//         }
//         std::vector<char> s2(state.begin(), state.end());
//         s2[i]--;
//         auto gen = gen_indices(s2, num_to_fix - 1, std::vector<int>(), max_threads);
//         while(gen) {

//           std::vector<int> ev(prepend.begin(), prepend.end());
//           ev.push_back(i);
//           auto h = gen();
//           ev.insert(ev.end(), h.begin(), h.end());
//           co_yield ev;
//         }
//       }
//     }
//     co_return;
// }

/*
**     The states for a given value:
**     ________        ________
**    |        |      |        |
** 4       3       2      1       0
*/


template<typename T>
inline std::vector<T> combine(std::vector<T> pre, T event, std::vector<T> post) {
  std::vector<T> e(pre.begin(), pre.end());
  e.push_back(event);
  e.insert(e.end(), post.begin(), post.end());
  return e;
}

inline std::set<int> possible_indices(std::vector<char> state, tid_t max_conc = MAX_THREADS) {
  if (std::all_of(state.begin(), state.end(), [](char i) {
      return i == 0;
  })) {
    return std::set<int>();
  }

  // Possible adds, just the lowest idx...
  size_t earliest_add;
  for(earliest_add = 0; earliest_add < state.size(); earliest_add++) {
    if(state[earliest_add] == 4)
      break;
  }

  size_t current_concurrency = std::count_if(state.begin(), state.end(), [](char i) {
    return i == 1 || i == 3;
  });

  std::set<int> possible_returns;
  for(int i = 0; i < state.size(); i++) {
    if(state[i] == 1 || state[i] == 3)
      possible_returns.insert(i);
  }

  std::set<int> pinds(possible_returns.begin(), possible_returns.end());

  if(current_concurrency < max_conc) {
    // All possible pop_calls
    for(size_t i = 0; i < state.size(); i++) {
      if(state[i] == 2)
        pinds.insert(i);
    }
    // The first possible push, if there is one
    if(earliest_add < state.size())
      pinds.insert(earliest_add);
  }

  return pinds;


}


inline Generator<std::vector<int>> gen_histories(std::vector<char> state, tid_t max_conc = MAX_THREADS, int num_to_fix = -1, std::vector<int> prepend = std::vector<int>()) {
  if(
    num_to_fix == 0 ||  // Partial finished
    std::all_of(state.begin(), state.end(), [](char a) { return a == 0; }) // Total Finished
  ) {
    co_yield prepend;
    co_return;
  }

  std::set<int> indices = possible_indices(state, max_conc);
  for(int i : indices) {
    std::vector<char> s2(state.begin(), state.end());
    s2[i]--;
    auto gen = gen_histories(s2, max_conc, num_to_fix - 1);
    while(gen) {
      co_yield combine(prepend, i, gen());
    }
  }
  co_return;
}


template<typename T>
inline T select_random(std::set<T> &set, std::mt19937_64 &rand) {
  std::vector<T> v(set.begin(), set.end());
  std::uniform_int_distribution<std::mt19937_64::result_type> dist(0, v.size() - 1);
  int r = dist(rand);
  return v[r];
}

inline std::vector<int> gen_single(std::vector<char> state, tid_t max_conc, std::mt19937_64 &rand, std::vector<int> prepend = std::vector<int>()) {
  std::set<int> indices = possible_indices(state, max_conc);

  while(!indices.empty()) {
    int selected = select_random(indices, rand);

    state[selected]--;
    prepend.push_back(selected);
    indices = possible_indices(state, max_conc);
  }
  return prepend;
}


// inline Generator<std::vector<ev_t>> gen_histories(std::vector<char> state, std::vector<ev_t> prepend = std::vector<ev_t>(), tid_t max_concurrency = MAX_THREADS) {
//     bool added_already = false;
//     if(std::all_of(state.begin(), state.end(), [](int i) {
//       return i == 0;
//     })) {
//       // All operations are done.
//       co_yield prepend;

//       co_return;
//     }

//     if(std::all_of(state.begin(), state.end(), [](int i) {
//       return i == 0 || i == 4;
//     })) {
//       int i = 0;
//       while(state[i] == 0)
//         i++;

//       std::vector<char> s2(state.begin(), state.end());
//       s2[i]--;
//       added_already = true;
//       auto gen = gen_histories(s2);
//       while(gen) {
//         std::vector<ev_t> ev(prepend.begin(), prepend.end());
//         ev.push_back(evt(i, state[i]));
//         auto o = gen();
//         ev.insert(ev.end(), o.begin(), o.end());
//         co_yield ev;
//       }
//       co_return;
//     }
//     for(int i = 0; i < state.size(); i++) {
//       int rem = state[i];
//       if(rem > 0) {
//         if(rem == 4) {
//           if(added_already)
//             continue;
//           else
//             added_already = true;
//         }
//         std::vector<char> s2(state.begin(), state.end());
//         s2[i]--;
//         auto gen = gen_histories(s2);
//         while(gen) {

//           std::vector<ev_t> ev(prepend.begin(), prepend.end());
//           ev.push_back(evt(i, state[i]));

//           auto h = gen();


//           ev.insert(ev.end(), h.begin(), h.end());
//           co_yield ev;
//         }
//       }
//     }
//     co_return;
// }

inline std::vector<ev_t> make_history(int size, int *events) {
  std::vector<ev_t> out;
  out.reserve(size * 4);
  std::vector<char> initial;
  initial.resize(size, 4);

  for(int i = 0; i < size * 4; i++) {
    out.push_back(evt(events[i], initial[events[i]]));
    initial[events[i]]--;
  }

  return out;
}

inline std::vector<ev_t> make_history(int size, std::vector<int> &events) {
  return make_history(size, events.data());
}

inline std::vector<int> create_single(int size, tid_t max_threads, std::mt19937_64 &rand) {
  std::vector<char> initial;
  initial.resize(size, 4);
  return gen_single(initial, max_threads, rand);
}

inline Generator<std::vector<int>> create_generator_prepended(int size, std::vector<int> &events, int num_new = INT_MAX, tid_t max_threads = MAX_THREADS) {
  std::vector<int> prepend;
  std::vector<char> initial;
  initial.resize(size, 4);
  for(auto v : events) {
    assert(v < initial.size());
    prepend.push_back(v);
    initial[v]--;
  }
  return gen_histories(initial, max_threads, num_new, prepend);

}

inline int generator_count(int num_fixed) {
if(num_fixed == 0)
    return 1;
  if(num_fixed == 1)
    return 1;
  if(num_fixed == 2)
    return 2;
  if(num_fixed == 3)
    return 5;
  return -1;

}

inline std::vector<std::vector<int>> create_inits(int size, int min_amt, tid_t max_threads = MAX_THREADS) {
  std::vector<std::vector<int>> histories { std::vector<int> {0}};
  std::vector<std::vector<int>> new_histories;
  int remaining = size * 4 - 1;
  while((histories.size() < min_amt) && remaining != 0) {
    remaining--;
    new_histories.clear();
    for(auto h : histories) {
      auto gen = create_generator_prepended(size, h, 1, max_threads);
      while(gen)
        new_histories.push_back(gen());
    }
    histories.clear();
    histories = new_histories;
  }
  return histories;
}

#endif // GENERATOR_H_
