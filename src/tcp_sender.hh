#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <set>
#include <queue>
#include <unordered_set>

// class OutBound
// {
//   unit64_t abseqno_;
//   TCPSenderMessage msg_;
//   unit64_t 
// }

class OutStanding
{
public:
  OutStanding(TCPSenderMessage msg_, uint64_t seqno): msg(msg_), seqno_(seqno), ttl_(0) {}
  TCPSenderMessage msg;
  uint64_t seqno_;
  uint64_t ttl_;
  bool operator< (const OutStanding& b) const { return seqno_ < b.seqno_; }
};


/*
  push 将reader内容进行segment，保存在outstandings_
  may_send 从中发送一个
  outstandings 应该是待发送的  set
  map <seqno, outs>

  sendings_ 存发送队列，并且为其成员记录 停留时间，以及重新发送的次数
  当接到ack，如何删除已完成的，因为有可能 (2, 1) < (3, 1) < (1, 2), 那么ack 1到来的时候就得遍历了


*/
class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t cur_RTO_ms_ {};
  bool begining_ {false};
  bool finished_ {false};
  uint64_t ackno_ {};
  uint64_t next_seqno_ {};
  uint16_t window_size_ {1};
  uint64_t retransmissions_ {};
  uint64_t num_seq_in_flight_ {};
  std::set<OutStanding> outstandings_ {};
  std::set<OutStanding> flight_ {};
  std::unordered_set<uint64_t> hash_ {};
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};

// 