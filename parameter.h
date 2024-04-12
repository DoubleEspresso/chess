/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
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


struct parameters {

  parameters() {}

  parameters(const parameters& o) { *this = o; }

  parameters& operator=(const parameters& o) {
    tempo = o.tempo;
    sq_score_scaling = o.sq_score_scaling;
    mobility_scaling = o.mobility_scaling;
    attack_scaling = o.attack_scaling;
    attacker_weight = o.attacker_weight;
    king_shelter = o.king_shelter;
    king_safe_sqs = o.king_safe_sqs;
    uncastled_penalty = o.uncastled_penalty;
    pinned_scaling = o.pinned_scaling;
    fixed_depth = o.fixed_depth;
    return *this;
  }

  float tempo = 0.3f;


  // square score parameters
  std::vector<float> sq_score_scaling{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

  // mobility tables
  std::vector<float> mobility_scaling{ 1.0f, 1.0, 1.0f, 1.0f, 0.0f };

  // piece attack tables
  std::vector<float> attack_scaling{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

  const float knight_attks[5] = { 3.0f, 9.0f, 9.45f, 14.4f, 27.3f };
  const float bishop_attks[5] = { 3.0f, 9.0f, 9.45f, 14.4f, 27.3f };
  const float rook_attks[5] = { 1.5f, 4.5f, 4.725f, 7.2f, 13.65f };
  const float queen_attks[5] = { 0.75f, 2.25f, 2.3625f, 3.6f, 6.825f };

  std::vector<float> trapped_rook_penalty{ 1.0f, 2.0f }; // mg, eg

  std::vector<float> attk_queen_bonus{ 2.0f, 1.0f, 1.0f, 1.0f, 0.0f };

  // piece pinned scale factors
  std::vector<float> pinned_scaling{ 1.0f, 1.0f, 2.0f, 3.0f, 4.0f };

  // minor piece bonuses
  std::vector<float> knight_outpost_bonus{ 0.0f, 1.0f, 2.0f, 3.0f, 3.0f, 2.0f, 1.0f, 0.0f };
  std::vector<float> bishop_outpost_bonus{ 0.0f, 0.0f, 1.0f, 2.0f, 2.0f, 1.0f, 0.0f, 0.0f };

  // king harassment tables
  const float knight_king[3] = { 1.0, 2.0, 3.0 };
  const float bishop_king[3] = { 1.0, 2.0, 3.0 };
  const float rook_king[5] = { 1.0, 2.0, 3.0, 3.0, 4.0 };
  const float queen_king[7] = { 1.0, 3.0, 3.0, 4.0, 4.0, 5.0, 6.0 };
  std::vector<float> attacker_weight { 0.5f, 4.0f, 8.0f, 16.0f, 32.0f };
  std::vector<float> king_shelter { -3.0f, -2.0f, 2.0f, 3.0f }; // 0,1,2,3 pawns
  std::vector<float> king_safe_sqs{ -4.0f, -2.0f, -1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 4.0f };
  float uncastled_penalty = 5.0f;
  const float connected_rook_bonus = 1.0f;
  const float doubled_bishop_bonus = 4.0f;
  const float open_file_bonus = 1.0f;
  const float bishop_open_center_bonus = 1.0f;
  const float bishop_color_complex_penalty = 1.0f;
  const float bishop_penalty_pawns_same_color = 1.0f;
  const float rook_7th_bonus = 2.0f;

  // pawn params
  const float doubled_pawn_penalty = 4.0f;
  const float backward_pawn_penalty = 1.0f;
  const float isolated_pawn_penalty = 2.0f;
  const float passed_pawn_bonus = 2.0f;
  const float semi_open_pawn_penalty = 1.0f;

  // move ordering
  const float counter_move_bonus = 100.0f; // 5
  const float threat_evasion_bonus = 100.0f; // 2

  // search params 
  int fixed_depth = -1;

  const float pawn_lever_score[64] =
  {
    1, 2, 3, 4, 4, 3, 2, 1,
    1, 2, 3, 4, 4, 3, 2, 1,
    1, 2, 3, 4, 4, 3, 2, 1,
    1, 2, 3, 4, 4, 3, 2, 1,
    1, 2, 3, 4, 4, 3, 2, 1,
    1, 2, 3, 4, 4, 3, 2, 1,
    1, 2, 3, 4, 4, 3, 2, 1,
    1, 2, 3, 4, 4, 3, 2, 1
  };
};


/*
template<typename T>
class tuneable_params {

  std::vector<parameter<T>> params;

public:
  params() {};


  void load(const std::string& filename);
  void save(const std::string& filename);
  void update(const std::vector<int> pbil_bits);


};
*/
#endif