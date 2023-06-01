#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>
#include <iostream>
using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return num_seq_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if (outstandings_.empty()) return {};
  else 
  {
    TCPSenderMessage msg = outstandings_.begin()->msg;
    flight_.insert(std::move(*outstandings_.begin()));
    outstandings_.erase(outstandings_.begin());
    return msg;
  }
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  if (finished_) return ;
  if (!begining_) cur_RTO_ms_ = initial_RTO_ms_;
  uint64_t rest_window = 0;
  // special case -- zero window_size
  if (window_size_ == 0 && num_seq_in_flight_ == window_size_) rest_window = 1;
  else if (window_size_ > num_seq_in_flight_) rest_window = window_size_ - num_seq_in_flight_;
  
  do
  {
    // add SYN at first run
    if (!begining_ && rest_window) rest_window -= 1;
    // fetch payload
    uint64_t readable = min((uint64_t)TCPConfig::MAX_PAYLOAD_SIZE, min(rest_window, outbound_stream.bytes_buffered()));
    string s(outbound_stream.peek().substr(0, readable));
    outbound_stream.pop(readable);
    rest_window -= readable;
    // FIN only live once
    bool FIN = outbound_stream.is_finished() && rest_window;
    if (FIN) finished_ = true;
    rest_window -= FIN;

    if (readable || !begining_ || FIN)
    {
      TCPSenderMessage msg{Wrap32::wrap(next_seqno_, isn_), !begining_, std::move(s), FIN};
      outstandings_.insert(OutStanding(msg, next_seqno_));
      next_seqno_ += msg.sequence_length();
      hash_.insert(next_seqno_);
      num_seq_in_flight_ += msg.sequence_length();
      begining_ = true;
    }
  } while (rest_window && outbound_stream.bytes_buffered());
}
 
TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  return {Wrap32::wrap(next_seqno_, isn_)};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if (msg.ackno.has_value()) 
  {
    uint64_t recv_ack = msg.ackno.value().unwrap(isn_, ackno_);
    // decide ack is one of the seqno generated from TcpSender
    if (hash_.count(recv_ack) == 0) return ;
    // update right edge
    if (ackno_ + window_size_ < recv_ack + msg.window_size) window_size_ = msg.window_size; 
    if (recv_ack <= ackno_) return ;

    cur_RTO_ms_ = initial_RTO_ms_;
    retransmissions_ = 0;
    ackno_ = recv_ack;
    window_size_ = msg.window_size;
    // delete acknoledged ones
    while (outstandings_.size())
    {
      uint64_t seqno = outstandings_.begin()->seqno_;
      if (seqno >= recv_ack) break;
      hash_.erase(seqno);
      num_seq_in_flight_ -= outstandings_.begin()->msg.sequence_length();
      outstandings_.erase(outstandings_.begin());
    }
    // delete them in resend flight_;
    while (flight_.size() && flight_.begin()->seqno_ < recv_ack){
      hash_.erase(flight_.begin()->seqno_);
      num_seq_in_flight_ -= flight_.begin()->msg.sequence_length();
      flight_.erase(flight_.begin());
    } 
  }
  else if (!msg.ackno.has_value()) window_size_ = msg.window_size; 
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if (flight_.size())
  {
    OutStanding o = std::move(*flight_.begin());
    flight_.erase(flight_.begin());
    o.ttl_ += ms_since_last_tick;

    if (o.ttl_ >= cur_RTO_ms_)
    {
      if (window_size_) cur_RTO_ms_ <<= 1;
      ++retransmissions_;
      o.ttl_ = 0;
      outstandings_.insert(std::move(o));
    } 
    else flight_.insert(std::move(o));
  }
}
