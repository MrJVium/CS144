#include "wrapping_integers.hh"
#include <iostream>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // std::cout << (uint32_t)(n + zero_point.raw_value_) << std::endl;
  return Wrap32(n + zero_point.raw_value_);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // 如果t大于=r, +mod and cur
  // 如果t < r, -mod and cur
  // 如果r < zero, +mod
  checkpoint += zero_point.raw_value_;
  uint64_t mod = 1UL << 32;
  uint64_t t = checkpoint % mod, r;
  if (t >= raw_value_)
    if (mod + raw_value_ - t > t - raw_value_) r = checkpoint - t + raw_value_;
    else r = checkpoint - t + mod + raw_value_ ;
  else
    if (checkpoint < mod || t + mod - raw_value_ > raw_value_ - t) r = checkpoint - t + raw_value_;
    else r = checkpoint - t - mod + raw_value_;
  return r - zero_point.raw_value_ + (r < zero_point.raw_value_ ? mod : 0);
}
