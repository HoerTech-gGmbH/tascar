#ifndef ASYNC_FILE_H
#define ASYNC_FILE_H

#include <stdint.h>
#include <string>
#include <sndfile.h>

#define INVALID_LOCATION (1<<30)

void lprintbuf(float* buf, unsigned int n);

#define printbuf(a,b) DEBUG(buf);lprintbuf(a,b)

namespace TASCAR {

  /**
     \brief Same as a sound file, but a loop count can be given

     The readf and seekf functions will behave as if the file would
     contain the looped number of data. If loopcnt == 0 then seekf and
     readf will always succeed.
   */
  class looped_sndfile_t {
  public:
    looped_sndfile_t(const std::string& fname,uint32_t loopcnt);
    ~looped_sndfile_t();
    uint32_t readf_float( float* buf, uint32_t frames );
    uint32_t seekf( uint32_t frame );
    uint32_t get_frames() const {return sf_inf.frames;};
    uint32_t get_loopedframes() const;
    uint32_t get_channels() const {return sf_inf.channels;};
  protected:
    SNDFILE* sfile;
    SF_INFO sf_inf;
    uint32_t loopcnt_;
    uint32_t filepos_looped;
  };

  class inftime_looped_sndfile_t : public looped_sndfile_t {
  public:
    inftime_looped_sndfile_t(const std::string& fname,uint32_t loopcnt);
    uint32_t readf_float( float* buf, uint32_t frames );
    int32_t seekf_inf( int32_t frame );
  private:
    int32_t virtual_filepos;
  };

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

  class async_sndfile_t {
  public:
    async_sndfile_t( uint32_t numchannels, uint32_t buffer_length, uint32_t fragsize );
    async_sndfile_t( const async_sndfile_t& src );
    ~async_sndfile_t();
    /**
       \brief real-time safe, return audio data if available

       \param firstframe sample position of first frame
       \param n number of frames
       \param channels number of channels (must match number of channels given in constructor)
       \param buf pointer on data pointers, one pointer for each channel
     */
    void request_data( int32_t firstframe, uint32_t n, uint32_t channels, float** buf );
    void open(const std::string& fname, uint32_t firstchannel, int32_t first_frame, double gain,uint32_t loop);
    void start_service();
    void stop_service();
    unsigned int get_xruns() {unsigned int xr(xrun);xrun=0;return xr;};
  private:
    void service();
    static void * service(void* h);
    //void rt_request_from_slave( uint32_t n );
    //void slave_read_file( uint32_t firstframe );
    bool service_running;
    bool run_service;
    uint32_t numchannels_;
    uint32_t buffer_length_;
    uint32_t fragsize_;
    ringbuffer_t rb;
    //pthread_mutex_t mtx_readrequest;
    //pthread_mutex_t mtx_wakeup;
    // mtx_file protects sfile, file_channel and filebuffer:
    pthread_mutex_t mtx_file;
    int32_t requested_startframe;
    bool need_data;
    inftime_looped_sndfile_t* sfile;
    uint32_t file_firstchannel;
    float* file_buffer;
    float* read_fragment_buf;
    float* disk_fragment_buf;
    uint32_t file_frames;
    uint32_t file_channels;
    int32_t file_firstframe;
    //int32_t file_lastframe;
    double gain_;
    //
    pthread_t srv_thread;
    unsigned int xrun;
    uint32_t min_read_chunk;
    //
  };

};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
