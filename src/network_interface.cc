#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

EthernetFrame make_frame1( const EthernetAddress& src,
                          const EthernetAddress& dst,
                          const uint16_t type,
                          vector<Buffer> payload )
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = std::move( payload );
  return frame;
}

ARPMessage make_arp1( const uint16_t opcode,
                     const EthernetAddress sender_ethernet_address,
                     const uint32_t& sender_ip_address,
                     const EthernetAddress target_ethernet_address,
                     const uint32_t& target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = sender_ethernet_address;
  arp.sender_ip_address = sender_ip_address;
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}


// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t dst_ip = next_hop.ipv4_numeric();
  if (ARP_table_.count(dst_ip)){
    EthernetFrame ip_frame = make_frame1(ethernet_address_, ARP_table_[dst_ip].first, EthernetHeader::TYPE_IPv4, serialize(dgram));
    outbound_Ethernet_frames_.push(std::move(ip_frame));
  } else {
    if (ip_ARP_ttl_.count(dst_ip) == 0) {
      ARPMessage arp = make_arp1(ARPMessage::OPCODE_REQUEST, ethernet_address_, ip_address_.ipv4_numeric(), {}, next_hop.ipv4_numeric());
      EthernetFrame ARP_frame = make_frame1(ethernet_address_, ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, serialize(std::move(arp)));
      ip_ARP_ttl_[dst_ip] = make_pair(5L * 1000, ARP_frame); // ttl 5s
      outbound_Ethernet_frames_.push(std::move(ARP_frame));
    }
    wait_for_arp_[dst_ip].push(std::move(dgram));
    // cout << "send_datagram: unkown " << Address::from_ipv4_numeric(dst_ip).ip() << endl;
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) return {};
  
  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram ip_datagram;
    if (parse(ip_datagram, frame.payload)) return ip_datagram;
    else return {};
  } else if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp;
    if (parse(arp, frame.payload)) {
      // check dst ip host
      if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric()) {
        ARPMessage ARP_reply = make_arp1(ARPMessage::OPCODE_REPLY, ethernet_address_, ip_address_.ipv4_numeric(), arp.sender_ethernet_address, arp.sender_ip_address);
        EthernetFrame ARP_frame = make_frame1(ethernet_address_, frame.header.src, EthernetHeader::TYPE_ARP, serialize(std::move(ARP_reply)));
        outbound_Ethernet_frames_.push(std::move(ARP_frame));
      }
      // remember
      ARP_table_[arp.sender_ip_address] = make_pair(arp.sender_ethernet_address, 30L * 1000);
      // check queue
      if (wait_for_arp_.count(arp.sender_ip_address)) {
        queue<InternetDatagram> q = std::move(wait_for_arp_[arp.sender_ip_address]);
        while (q.size()){
          InternetDatagram dgram = q.front(); q.pop();
          EthernetFrame ip_frame = make_frame1(ethernet_address_, ARP_table_[arp.sender_ip_address].first, EthernetHeader::TYPE_IPv4, serialize(dgram));
          outbound_Ethernet_frames_.push(std::move(ip_frame));
        }
        wait_for_arp_.erase(arp.sender_ip_address);
      }
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
    for (auto it = ip_ARP_ttl_.begin(); it != ip_ARP_ttl_.end();){
      if (it->second.first <= ms_since_last_tick){
        if (ARP_table_.count(it->first) == 0){ // retransmit
          outbound_Ethernet_frames_.push(it->second.second);
          it->second.first = 5L * 1000;
          ++it;
        }
        else ip_ARP_ttl_.erase(it ++);
      } 
      else it->second.first -= ms_since_last_tick, ++it; // 防溢出
    }
    for (auto it = ARP_table_.begin(); it != ARP_table_.end();){
      if (it->second.second <= ms_since_last_tick) ARP_table_.erase(it ++);
      else it->second.second -= ms_since_last_tick, ++it; // 防溢出
    }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if (outbound_Ethernet_frames_.size()) {
    auto frame = outbound_Ethernet_frames_.front();
    outbound_Ethernet_frames_.pop();
    return frame;
  }
  return {};
}
