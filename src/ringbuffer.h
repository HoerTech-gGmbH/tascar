#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>
#include <pthread.h>

#define INVALID_LOCATION (1<<30)

namespace TASCAR {

  /**
   * \brief Relocatable ring buffer
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
    //void set_locate(int32_t l){requested_location = l;};
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
