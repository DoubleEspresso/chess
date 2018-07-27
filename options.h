#pragma once

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>


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
  
  template<typename T> T value(const char * s) {
    std::unique_lock<std::mutex> lock(m);
    std::istringstream ss{ opts[std::string{s}] };
    T v;
    ss >> v;
    lock.unlock();
    return v;
  }
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

extern std::unique_ptr<options> opts;

#endif
