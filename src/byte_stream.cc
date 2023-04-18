#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), buffer(), cumulation_in( 0 ), cumulation_out( 0 ), end_( false ), error_( false )
{}

void Writer::push( string data )
{
  // Your code here.
  size_t available_capacity_ = available_capacity();
  if ( end_ || available_capacity_ <= 0 )
    return;
  int len_ = min( available_capacity_, (uint64_t)data.size() );
  buffer.write( data.substr( 0, len_ ) );
  cumulation_in += len_;
}

void Writer::close()
{
  // Your code here.
  end_ = true;
}

void Writer::set_error()
{
  // Your code here.
  error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return end_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer.readableBytes();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return cumulation_in;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view( buffer.peek(), buffer.readableBytes() );
}

bool Reader::is_finished() const
{
  // Your code here.
  return end_ && buffer.readableBytes() == 0;
}

bool Reader::has_error() const
{
  // Your code here.
  return error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  cumulation_out += buffer.retrieve( len );
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer.readableBytes();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return cumulation_out;
}
