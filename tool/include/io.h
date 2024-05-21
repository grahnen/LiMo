#ifndef IO_H
#define IO_H
#include <string>
#include <regex>
#include "monitor.hpp"
#include "typedef.h"

using Sequence = std::vector<event_t>;

//Reads input formatted like in violin,
// except each unique id becomes a thread


struct Configuration {
public:
  Sequence history;
  ADT type;
  tid_t num_threads = 0;
  bool needs_simpl;
  Configuration(Sequence history, ADT type, tid_t n_threads = 0, bool needs_simpl = false)
    : history(history), num_threads(n_threads), needs_simpl(needs_simpl), type(type) {}
  Configuration() : Configuration(Sequence(), unknown, 0, false) {}
};

const std::regex type_regex = std::regex("# @object ([a-z\\-]+).*");

const std::regex call_regex("[ \\t ]*\\[(\\d+)\\] call ([a-zA-Z]+)(.*)");
const std::regex ret_regex("[ \\t ]*\\[(\\d+)\\] return ?(.*)");

const std::regex crash_regex("crash");

// Strip parens
// ^(\()*([^\(^\)]*)(\))*$
// Match ?<t,i>
// ^([\d+\w+\?]+)<(\d+),\s?(\d+)>
const std::regex thr_match("^\\(? ?([_\\d+\\w+\\?]+)<(\\d+), ?(\\d+)> ?\\)?");
const std::regex val_match("^\\(? ?([_\\d+\\w+]+) ?\\)?");

const std::regex clean_regex("[ \\t ]*\\(*([a-zA-Z_\\d]*)\\)*");
// Match v

const std::regex new_call("[ \\t]*\\[(\\d+)\\] call ([a-z]+)(\\(([_a-z\\d]*)\\)| ?([_a-z\\d]*))");
const std::regex log_ret("[ \\t]*\\[(\\d+)\\] return ?([a-z\\d_]*)");
const std::regex strip_paren("[ \\t]*\\(([_a-zA-Z\\d]*)\\)");


const std::regex int_regex("(\\d+)");

const ParseMap get_parsers();

static int VAR_CTR = 0;

std::optional<int> input_from(std::string s, std::map<std::string, int> &varmap);

std::optional<int> read_arg(std::string arg, std::map<std::string, int> &varmap);

std::optional<event_t> get_event(const char *line, std::map<tid_t, int> &counts, std::map<int, val_t> &value_map, std::map<std::string, int> &varmap, bool *needs_simpl);

Configuration *read_log(std::string filename);

Sequence *read_file(std::string filename);

void write_file(Sequence *h, std::string adt, std::string filename);

#endif
