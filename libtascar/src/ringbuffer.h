/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>
#include <pthread.h>

#define INVALID_LOCATION (1<<30)

namespace TASCAR {

  /**
   * \brief Relocatable ring buffer.
   *
   * The reading end can request a position, with the set_locate()
   * member. The writing end can detect a relocation request with the
   * member function relocation_requested(). read() and read_skip()
   * will read from the ring buffer, write() and write_zeros() can be
   * used to write to the ring buffer.
   *
   * Typical usage:
   * 
   * reading end (e.g., jack process callback):
   * <ol>
   * <li>set_locate()</li>
   * <li>read()</li>
   * </ol>
   *
   * writing end (e.g., disk buttler thread):
   * <ol>
   * <li> relocation_requested()<ul>
   * <li> lock_relocate()</li>
   * <li> get_requested_location()</li>
   * <li> (read from sound file or whatever)</li>
   * <li> unlock_relocate()</li></ul>
   * <li> write_space()</li>
   * <li> write()</li>
   * </ol>
   */
  class ringbuffer_t {
  public:
    ringbuffer_t(uint32_t size,uint32_t channels);
    ~ringbuffer_t();
    uint32_t read( float* buf, uint32_t frames, int32_t* current_loc=NULL );
    uint32_t write( float* buf, uint32_t frames );
    uint32_t read_skip( uint32_t frames, int32_t* current_loc=NULL );
    uint32_t write_zeros( uint32_t frames );
    uint32_t read_space();
    uint32_t write_space();
    void set_locate(int32_t l);
    bool relocation_requested(){return requested_location != INVALID_LOCATION;};
    int32_t get_requested_location(){return requested_location;};
    int32_t get_current_location(){return current_location;};
    void lock_relocate();
    void unlock_relocate();
  private:
    class pos_t {
    public:
      pos_t(uint32_t buflen):r(0),w(1),l(buflen){};
      uint32_t rspace();
      uint32_t wspace();
      uint32_t r;
      uint32_t w;
      uint32_t l;
    };
    pos_t get_pos(){return pos;};
    void reset();
    float* data;
    pos_t pos;
    uint32_t channels;
    int32_t current_location;
    int32_t requested_location;
    pthread_mutex_t mtx_write_access;
    pthread_mutex_t mtx_read_access;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
