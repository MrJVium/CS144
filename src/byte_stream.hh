#pragma once

#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
class Reader;
class Writer;

class ByteStream
{
protected:
  uint64_t capacity_;
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.

  class Buffer
  {
  public:
    static const size_t kInitialSize = 1024;
    Buffer() : readerIndex_( 0 ), writerIndex_( 0 ), buffer_( kInitialSize ) {};

    size_t write( const std::string& data )
    {
      ensureWritable( data.size() );
      size_t t = writerIndex_;
      for ( auto& c : data )
        buffer_[writerIndex_++] = c;
      return writerIndex_ - t;
    }

    const char* peek() const { return &*buffer_.begin() + readerIndex_; }

    void ensureWritable( size_t len )
    {
      if ( writableBytes() < len ) {
        makeSpace( len );
      }
    }

    size_t retrieve( size_t len )
    {
      size_t readableBytes_ = readableBytes();
      if ( readableBytes_ < len ) {
        readerIndex_ = 0;
        writerIndex_ = 0;
        return readableBytes_;
      } else {
        readerIndex_ += len;
        return len;
      }
    }
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t reusableBytes() const { return readerIndex_; }

  private:
    void makeSpace( size_t len )
    {
      if ( reusableBytes() + writableBytes() < len ) {
        buffer_.resize( writerIndex_ + len );
      } else {
        std::copy( std::make_move_iterator( buffer_.begin() + readerIndex_ ),
                   std::make_move_iterator( buffer_.begin() + writerIndex_ ),
                   buffer_.begin() );
        writerIndex_ = writerIndex_ - readerIndex_;
        readerIndex_ = 0;
      }
    }
    uint64_t readerIndex_;
    uint64_t writerIndex_;
    std::vector<char> buffer_;
  };

  Buffer buffer;
  uint64_t cumulation_in;
  uint64_t cumulation_out;
  bool end_;
  bool error_;

public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // Push data to stream, but only as much as available capacity allows.

  void close();     // Signal that the stream has reached its ending. Nothing more will be written.
  void set_error(); // Signal that the stream suffered an error.

  bool is_closed() const;              // Has the stream been closed?
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now?
  uint64_t bytes_pushed() const;       // Total number of bytes cumulatively pushed to the stream
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // Peek at the next bytes in the buffer
  void pop( uint64_t len );      // Remove `len` bytes from the buffer

  bool is_finished() const; // Is the stream finished (closed and fully popped)?
  bool has_error() const;   // Has the stream had an error?

  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream
};

/*
 * read: A (provided) helper function thats peeks and pops up to `len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t len, std::string& out );
