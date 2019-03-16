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
  std::unique_ptr<T> value;

  T update_val() {
    const auto val = bits.to_ulong();
    memcpy(value.get(), &val, sizeof(T));
  }

public:

  parameter<T>(T&& in, std::string& s) : tag(s) 
  { 
    value = util::make_unique<T>(in);
    bits = *reinterpret_cast<unsigned long*>(value.get());
  }
  parameter<T>(T& in, std::string& s) : tag(s) { set(in); }
  parameter<T>(T& in) : tag("") { set(in); }
  parameter<T>(const parameter<T>& o) { tag = o.tag;  set(*o.value); }
  virtual ~parameter<T>() {}

  parameter<T>& operator=(const parameter<T>& o) { tag = o.tag; set(*o.value); }
  T& operator()() { return *value; }

  inline void set(T& in) {
    memcpy(value.get(), &in, sizeof(T)); 
    bits = *reinterpret_cast<unsigned long*>(value.get());
  }

  inline std::bitset<sizeof(T) * CHAR_BIT> get_bits() { return bits; }
  inline T get() { return *value; }
  inline void print_bits() { std::cout << bits << std::endl; }
  inline void print() {
    std::cout << "tag: " << tag
      << " val " << *value
      << " bits " << bits << std::endl;
  }

};


#endif