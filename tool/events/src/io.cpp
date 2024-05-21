#include "io.h"
#include "util.h"
#include "parser.h"
#include <fstream>
#include <iostream>
#include <optional>

std::optional<val_t> str_to_val(std::string s_in, tid_t thr, std::map<tid_t, int> &m, std::map<int, val_t> &valmap, std::map<std::string, int> &varmap, bool *needs_simpl) {
  std::cmatch cm;

  std::string s = s_in;
  while(std::regex_match(s.c_str(), cm, clean_regex) && strcmp(s.c_str(), cm[1].str().c_str()) != 0) {
    s = cm[1];
  }

  if(strcmp(s.c_str(), "") == 0) {
    return {};
  }

  if(strcmp(s.c_str(), "empty") == 0)
    return {};

  if(strcmp(s.c_str(), "Empty") == 0)
    return {};

  if(std::regex_match(s.c_str(), cm, thr_match)) {
    char val[10];
    strcpy(val, cm[1].str().c_str());
    val_t v(val, std::stoi(cm[2]), std::stoi(cm[3]));
    std::cout << v << std::endl;
    return v;
  }

  std::optional<int> i = input_from(s, varmap);

  if (i.has_value()) {
    *needs_simpl = true;
    if (valmap.contains(i.value())) {
      return valmap[i.value()];
    }

    int idx = 0;
    if (m.contains(thr))
      idx = m[thr];

    val_t v(s.c_str(), thr, idx);
    m[thr] = idx + 1;

    valmap[i.value()] = v;
    return v;
  }



  return {};
}

std::optional<event_t> get_event(const char *line, std::map<tid_t, int> &counts, std::map<int, val_t> &value_map, std::map<std::string,int> &varmap, bool* needs_simpl) {
  std::cmatch cm;
  std::regex c_regex = call_regex;
  std::regex r_regex = ret_regex;
  
  if (std::regex_match(line, cm, c_regex)) {
    tid_t tid = read_arg(cm[1], varmap).value();
    std::string op = cm[2].str();
    std::string argstr = cm[3];

    optval_t arg = str_to_val(argstr, tid, counts, value_map, varmap,  needs_simpl);

    event_t e = parse_event(tid, op, arg);
    if(e.type == Epop && !e.val.has_value())
      *needs_simpl = true;
    return e;
  } else if (std::regex_match(line, cm, r_regex)) {
    int tid = read_arg(cm[1], varmap).value();
    optval_t arg = str_to_val(cm[2], tid, counts, value_map, varmap, needs_simpl);
    if(arg.has_value()) {
      *needs_simpl = true;
    }
    return parse_event(tid, "return", arg);
  } else if (std::regex_match(line, cm, crash_regex)) {
    event_t ev = parse_event(0, "crash", {});
    return ev;
  }
  if(strcmp(line, "") != 0) {
    std::cout << "Failed parsing: " << line << std::endl;
  }
  
  return {};
}

ADT parse_type(std::string type) {
  if(type.compare("stack") == 0 || type.compare("atomic-stack") == 0) {
    return stack;
  }
  else if(type.compare("queue") == 0 || type.compare("atomic-queue") == 0) {
    return queue;
  }

  else {
    throw std::logic_error("Unknown ADT: " + type);
  }
};

Configuration *read_log(std::string filename) {

  std::regex c_regex = new_call;
  std::regex r_regex = log_ret;

  std::fstream file;
  file.open(filename, std::ios::in);

  std::map<tid_t, int> counts;
  std::map<int, val_t> value_map;
  std::map<std::string, int> varmap;
  
  Configuration *c = new Configuration();
  char line[100];
  std::cmatch cm;

  //Get data type
  while (file.getline(line, 100)) {
    if (std::regex_match(line, cm, type_regex)) {
      c->type = parse_type(cm[1].str());
      break;
    }
  }
  //Get history
  //std::cout << "Reading <" << cm[1] << "> history" << std::endl;
  while (file.getline(line, 100)) {
    std::optional<event_t> e = get_event(line, counts, value_map, varmap, &c->needs_simpl);
    if(e.has_value()) {
      if((e->thread  + 1) > c->num_threads)
        c->num_threads = e->thread + 2;
      c->history.push_back(e.value());
    }
  }
  file.close();
  return c;
}

std::optional<int> input_from(std::string s, std::map<std::string, int> &varmap) {
  std::cmatch cm;
  if(std::regex_match(s.c_str(), cm, int_regex)) {
    int v = stoi(cm[1]);
    return v;
    // std::cout << "Assign " << s << " to " << nval << std::endl
  }

  if (varmap.count(s) == 0) {
      varmap[s] = VAR_CTR++;
  }

  return varmap[s];
}

std::optional<int> read_arg(std::string arg, std::map<std::string, int> &varmap) {
  std::cmatch cm;
  std::string tmp = arg;
  while(std::regex_match(tmp.c_str(), cm, strip_paren)) {
    tmp = cm[1];
  }

  return input_from(tmp, varmap);

}

std::string violin_compatible(event_t &ev) {
  std::stringstream ss, vs;
  std::optional<std::string> s = std::nullopt;

  if (ev.val.has_value()) {
    vs << ev.val->thread << "r" << ev.val->idx;
    s = vs.str();
  }

  ss << "[" << ev.thread << "] " << event_str(ev.type, s);

  return ss.str();
}

void write_file(Sequence *h, std::string adt, std::string filename) {
  std::fstream f;
  f.open(filename, std::ios::out);
  f << "# @object " << adt << std::endl;

  for (int i = 0; i < h->size(); i++) {
    f << violin_compatible((*h)[i]) << std::endl;
  }
}
