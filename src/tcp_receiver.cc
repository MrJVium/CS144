#include "tcp_receiver.hh"
#include <iostream>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if (message.SYN && has_ISN_ == false) zero_point = message.seqno, has_ISN_ = true;
  uint64_t first_index = message.seqno.unwrap(zero_point, ackno);
  if (message.SYN) ++ first_index;
  reassembler.insert(first_index - 1, message.payload.release(), message.FIN, inbound_stream);
  ackno = reassembler.get_next_index() + has_ISN_ + inbound_stream.is_closed();
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  uint16_t window_size = min((uint64_t)UINT16_MAX, inbound_stream.available_capacity());
  if (has_ISN_) return {Wrap32::wrap(ackno, zero_point), window_size};
  else return {nullopt, window_size};
}
