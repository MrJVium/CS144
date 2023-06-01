#include "reassembler.hh"
#include <iostream>
#include <iterator>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  if (output.is_closed()) return ;
  if (is_last_substring) last_index = first_index + data.size() - 1, finish_ = true;

  uint64_t available_capacity_ = output.available_capacity();
  uint64_t idx = next_index - first_index;
  if (next_index >= first_index && idx < data.size()) // 有东西能存
  { 
    if (available_capacity_ > 0)
    {
      uint64_t writable = min(available_capacity_, data.size() - idx);
      output.push(std::move(data.substr(idx, writable)));
      next_index += writable;
      if (is_last_substring)
      {
        output.close();
        return ;
      } 
      string s;
      while (remains.size())
      {
        auto it = remains.lower_bound({next_index, ""});
        if (it == remains.begin() && it->first > next_index) break;
        if (it == remains.end() || it->first > next_index) --it;
        uint64_t l = it->first, r = it->first + it->second.size() - 1;
        if (l <= next_index && next_index <= r)
        {
          s += it->second.substr(next_index - l); // 因为保存的不能超过 available_capacity_ - 1, 所以next_index 只要还在范围内 就能读。
          next_index = r + 1;
        } 
        bytes_pending_ -= it->second.size();
        remains.erase(it);
        if (finish_ && next_index > last_index) break;
      }

      if (s.size()) output.push(std::move(s));
    }
    else {} // discard 
  }
  else if (data.size() > 0 && first_index > next_index && bytes_pending_ < available_capacity_ - 1)
  {
    if (finish_ && first_index > last_index) return ;
    if (first_index > next_index + available_capacity_) return ;
    data = data.substr(0, next_index + available_capacity_ - first_index);
    uint64_t l = first_index, r = first_index + data.size() - 1;

    while (remains.size())
    {
      auto it = remains.lower_bound({l, ""});
      if (it == remains.begin() && it->first > r) break;
      if (it == remains.end() || it->first > r) --it;
      uint64_t ll = it->first, rr = it->first + it->second.size() - 1;
      if (ll <= l && rr >= r) return ;
      else if (ll <= r && rr > r)
      {
        data += it->second.substr(r + 1 - ll);
        bytes_pending_ -= it->second.size();
        remains.erase(it);
        r = rr;
      }
      else if (ll >= l && rr <= r)
      {
        bytes_pending_ -= it->second.size();
        remains.erase(it);
      }
      else if (ll < l && rr >= l)
      {
        data = it->second + data.substr(rr - l + 1);
        bytes_pending_ -= it->second.size();
        remains.erase(it);
        l = ll;
      }
      else if (rr < l) break;
    }
    bytes_pending_ += data.size();
    remains.insert({l, std::move(data)});
  }
  if (finish_ && next_index >= last_index + 1) output.close(); // +1 防止unsigned 溢出
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}