#pragma once
#ifndef PARAMETER_H
#define PARAMETER_H

#include <bitset>
#include <string>
#include <iostream>


template<typename T>
class parameter {

protected:
  std::string tag;
  std::bitset<sizeof(T) * CHAR_BIT> bits;
  T * value;

public:

  parameter<T>(T& in, std::string& s) : tag(s) { set(in); }
  parameter<T>(T& in) : tag("") { set(in); }
  parameter<T>(const parameter<T>& o) { tag = o.tag;  set(*o.value); }
  virtual ~parameter<T>() {}

  parameter<T>& operator=(const parameter<T>& o) { tag = o.tag; set(*o.value); }
  T& operator()() { return *value; }


  inline void set(T& in) {
    value = &in;
    bits = *reinterpret_cast<unsigned long*>(value);
  }

  inline T get() { return *value; }
  inline void print_bits() { std::cout << bits << std::endl; }
  inline void print() {
    std::cout << "tag: " << tag
      << " val " << *value
      << " bits " << bits << std::endl;
  }

};


#endif