#pragma once

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <fstream>

#include "utils.h"
#include "evaluate.h"

class options {

 private:  
  std::mutex m;
  std::map<std::string, std::string> opts;
  void load_args(int argc, char * argv[]);
  
 public:
  options() = delete;
  options(const options&) = delete;
  options(const options&&) = delete;
  options& operator=(const options&) = delete;
  options& operator=(const options&&) = delete;
  
  options(int argc, char* argv[]) { load_args(argc, argv); }
  ~options() {}
  
  template<typename T> 
  inline T value(const char * s) {
    std::unique_lock<std::mutex> lock(m);
    std::istringstream ss{ opts[std::string{s}] };
    T v;
    ss >> v;
    return v;
  }

  template<typename T> 
  inline void set(const std::string key, const T value) {
    std::unique_lock<std::mutex> lock(m);
    std::string vs = std::to_string(value);
    if (opts.find(key) == opts.end()) {
      opts.emplace(key, vs);
    }
    else opts[key] = vs;
  }

  bool read_param_file(const std::string& filename);
  bool save_param_file();
  void set_engine_params();
};

inline void options::load_args(int argc, char * argv[]) {  
  
  auto matches = [](std::string& s1, const char* s2) { return strcmp(s1.c_str(), s2) == 0; };
  auto set = [this](std::string& k, std::string& v) { this->opts.emplace(k.substr(1), v); };
  
  for (int j=1; j < argc-1; j += 2) {
    std::string key = argv[j]; std::string val = argv[j+1];
    if (matches(key, "-threads")) set(key, val);
    else if (matches(key, "-book")) set(key, val);
    else if (matches(key, "-hashsize")) set(key, val);
    else if (matches(key, "-tune")) set(key, val);
    else if (matches(key, "-bench")) set(key, val);
  }
}


inline bool options::read_param_file(const std::string& filename) {
  std::string line("");
  std::ifstream param_file(filename);
  auto set = [this](std::string& k, std::string& v) { this->opts.emplace(k, v); };

  while (std::getline(param_file, line)) {
    
    // assumed format "param-tag:param-value"
    std::vector<std::string> tokens = util::split(line, ':');

    if (tokens.size() != 2) {
      std::cout << "skipping invalid line" << line << std::endl;
      continue;
    }

    set(tokens[0], tokens[1]);
    //std::cout << "stored param: " << tokens[0] << " value: " << tokens[1] << std::endl;
  }

  set_engine_params();
}

inline bool options::save_param_file() {
  std::ofstream param_file("engine.conf", std::ofstream::out);

  for (const auto& p : opts) {
    std::string line = p.first + ":" + p.second + "\n";
    param_file << line; 
    std::cout << "saved " << line << " into engine.conf " << std::endl;
  }
  param_file.close();

}


inline void options::set_engine_params() {
  auto matches = [](const std::string& s1, const char* s2) { return strcmp(s1.c_str(), s2) == 0; };
  using namespace eval;

  for (const auto& p : opts) {
    std::cout << "engine param: " << p.first << " = " << p.second << std::endl;
    if (matches(p.first, "tempo")) Parameters.tempo = value<float>("tempo");
    else if (matches(p.first, "pawn ss")) Parameters.sq_score_scaling[pawn] = value<float>("pawn ss");
    else if (matches(p.first, "knight ss")) Parameters.sq_score_scaling[knight] = value<float>("knight ss");
    else if (matches(p.first, "bishop ss")) Parameters.sq_score_scaling[bishop] = value<float>("bishop ss");
    else if (matches(p.first, "rook ss")) Parameters.sq_score_scaling[rook] = value<float>("rook ss");
    else if (matches(p.first, "queen ss")) Parameters.sq_score_scaling[queen] = value<float>("queen ss");
    else if (matches(p.first, "king ss")) Parameters.sq_score_scaling[king] = value<float>("king ss");
    else if (matches(p.first, "pawn ms")) Parameters.mobility_scaling[pawn] = value<float>("pawn ms");
    else if (matches(p.first, "knight ms")) Parameters.mobility_scaling[pawn] = value<float>("knight ms");
    else if (matches(p.first, "bishop ms")) Parameters.mobility_scaling[pawn] = value<float>("bishop ms");
    else if (matches(p.first, "rook ms")) Parameters.mobility_scaling[pawn] = value<float>("rook ms");
    else if (matches(p.first, "queen ms")) Parameters.mobility_scaling[pawn] = value<float>("queen ms");
    else if (matches(p.first, "pawn as")) Parameters.attack_scaling[pawn] = value<float>("pawn as");
    else if (matches(p.first, "knight as")) Parameters.attack_scaling[pawn] = value<float>("knight as");
    else if (matches(p.first, "bishop as")) Parameters.attack_scaling[pawn] = value<float>("bishop as");
    else if (matches(p.first, "rook as")) Parameters.attack_scaling[pawn] = value<float>("rook as");
    else if (matches(p.first, "queen as")) Parameters.attack_scaling[pawn] = value<float>("queen as");
  }
}

extern std::unique_ptr<options> opts;

#endif
