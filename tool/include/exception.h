#ifndef EXCEPTION_H
#define EXCEPTION_H
#include <stdexcept>
#include <string>

class Exception : public std::runtime_error {
 public:
  inline Exception(std::string what) : std::runtime_error(what) {}
};

class Violation : public Exception {
public:
  inline Violation(std::string what) : Exception(what) {}
};

class Crash : public Exception {
public:
  inline Crash(std::string what) : Exception(what) {}
};

#endif
