#include "ringbuffer.h"
#include <algorithm>
#include <string.h>

TASCAR::ringbuffer_t::ringbuffer_t(uint32_t size,uint32_t channels_)
  : data(new float[size*channels_]),
    pos(size),
    channels(channels_)
{
  reset();
  pthread_mutex_init( &mtx_read_access, NULL );
  pthread_mutex_init( &mtx_write_access, NULL );
}
 
TASCAR::ringbuffer_t::~ringbuffer_t()
{
  pthread_mutex_trylock( &mtx_write_access );
  pthread_mutex_unlock( &mtx_write_access);
  pthread_mutex_destroy( &mtx_write_access );
  pthread_mutex_trylock( &mtx_read_access );
  pthread_mutex_unlock( &mtx_read_access);
  pthread_mutex_destroy( &mtx_read_access );
  delete [] data;
}
uint32_t TASCAR::ringbuffer_t::pos_t::rspace()
{
  if( w >= r ) return w-r;
  return w+l-r;
}

uint32_t TASCAR::ringbuffer_t::pos_t::wspace()
{
  if( r > w ) return r-w-1;
  return r+l-w-1;
}

/**
 * \brief Return read space of ringbuffer, or zero if lock cannot be acquired.
 */
uint32_t TASCAR::ringbuffer_t::read_space()
{
  if( pthread_mutex_trylock( &mtx_read_access ) == 0 ){
    uint32_t retv(get_pos().rspace());
    pthread_mutex_unlock( &mtx_read_access );
    return retv;
  }
  return 0;
}

/**
 * \brief Return write space of ringbuffer, or zero if lock cannot be acquired.
 */
uint32_t TASCAR::ringbuffer_t::write_space()
{
  if( pthread_mutex_trylock( &mtx_write_access ) == 0 ){
    uint32_t retv(get_pos().wspace());
    pthread_mutex_unlock( &mtx_write_access );
    return retv;
  }
  return 0;
}

/**
 * \brief Read data from ringbuffer
 *
 * \param buf Data buffer, or NULL to ignore data (see TASCAR::ringbuffer_t::read_skip)
 * \param frames Number of frames to read
 * \param current_loc Pointer on current location, or NULL
 * \return Number of frames read
 */
uint32_t TASCAR::ringbuffer_t::read( float* buf, uint32_t frames, int32_t* current_loc )
{
  if( current_loc )
    *current_loc = INVALID_LOCATION;
  if( pthread_mutex_trylock( &mtx_read_access ) == 0 ){
    if( current_loc )
      *current_loc = current_location;
    pos_t tmp_p(get_pos());
    uint32_t frames_to_read = std::min(frames,tmp_p.rspace());
    uint32_t r1(std::min(tmp_p.r+frames_to_read,tmp_p.l)-tmp_p.r);
    tmp_p.r += frames_to_read;
    if( r1 && buf )
      memcpy( buf, &(data[pos.r*channels]), r1*sizeof(float)*channels);
    if( r1 < frames_to_read ){
      if( buf )
        memcpy( &(buf[r1*channels]), data, (frames_to_read-r1)*sizeof(float)*channels);
      tmp_p.r = (frames_to_read-r1);
    }
    pos.r = tmp_p.r;
    current_location += frames_to_read;
    pthread_mutex_unlock( &mtx_read_access );
    return frames_to_read;
  }
  return 0;
}

/**
 * \brief Advance ringbuffer read position without returning data
 *
 * \param frames Number of frames to be skipped
 * \param current_loc Pointer on current location, or NULL
 * \return Number of frames read
 */
uint32_t TASCAR::ringbuffer_t::read_skip( uint32_t frames, int32_t* current_loc )
{
  return read( NULL, frames, current_loc );
}

/**
 * \brief Write data to ringbuffer
 *
 * \param buf Data buffer, or NULL to write zeros
 * \param frames Number of frames to write
 * \return Number of frames written
 */
uint32_t TASCAR::ringbuffer_t::write( float* buf, uint32_t frames )
{
  if( pthread_mutex_trylock( &mtx_write_access ) == 0 ){
    pos_t tmp_p(get_pos());
    uint32_t frames_to_write = std::min(frames,tmp_p.wspace());
    uint32_t r1(std::min(tmp_p.w+frames_to_write,tmp_p.l)-tmp_p.w);
    tmp_p.w += frames_to_write;
    if( r1 ){
      if( buf )
        memcpy( &(data[pos.w*channels]), buf, r1*sizeof(float)*channels);
      else
        memset( &(data[pos.w*channels]), 0, r1*sizeof(float)*channels);
    }
    if( r1 < frames_to_write ){
      if( buf )
        memcpy( data, &(buf[r1*channels]), (frames_to_write-r1)*sizeof(float)*channels);
      else
        memset( data, 0, (frames_to_write-r1)*sizeof(float)*channels);
      tmp_p.w = (frames_to_write-r1);
    }
    pos.w = tmp_p.w;
    pthread_mutex_unlock( &mtx_write_access );
    return frames_to_write;
  }
  return 0;
}

uint32_t TASCAR::ringbuffer_t::write_zeros( uint32_t frames )
{
  return write( NULL, frames );
}

void TASCAR::ringbuffer_t::reset()
{
  pos.r = 0;
  pos.w = 0;
  current_location = 0;
  requested_location = INVALID_LOCATION;
}

void TASCAR::ringbuffer_t::lock_relocate()
{
  pthread_mutex_lock( &mtx_read_access );
  pthread_mutex_lock( &mtx_write_access );
}

void TASCAR::ringbuffer_t::unlock_relocate()
{
  uint32_t new_location(requested_location);
  reset();
  current_location = new_location;
  pthread_mutex_unlock( &mtx_read_access );
  pthread_mutex_unlock( &mtx_write_access );
}

void TASCAR::ringbuffer_t::set_locate(int32_t l)
{
  requested_location = l;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
