#pragma once

#include <bitset>
#include <string>
#include <iostream>


template<typename T>
class parameter {

protected:
  std::string tag;
  std::bitset<sizeof(T) * CHAR_BIT> bits;
  T value;

public:

  parameter<T>(T& in, std::string& s) : tag(s) { set(in); }
  parameter<T>(T& in) : tag("") { set(in);  }
  parameter<T>(const parameter<T>& o) { tag = o.tag;  set(value); }
  parameter<T>& operator=(const parameter<T>& o) { tag = o.tag; set(o.value); }
  virtual ~parameter<T>() {}

  inline void set(T& in) { 
    value = in; 
    bits = *reinterpret_cast<unsigned long*>(&value);
  }
  inline void adjust() { value += 2; }
  inline T get() { return value; }
  inline void print_bits() { std::cout << bits << std::endl;  }
  inline void print() {
    std::cout << "tag: " << tag
      << " val " << value
      << " bits " << bits << std::endl;
  }
};

template<typename T>
class evaluate_params : public parameter<T> {
public:
  evaluate_params<T>(const T& in) : parameter(in) { }
  ~evaluate_params<T>() {}
};