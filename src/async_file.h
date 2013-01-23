#ifndef ASYNC_FILE_H
#define ASYNC_FILE_H

#include <stdint.h>
#include <string>
#include <sndfile.h>

namespace TASCAR {

  class chunk_t {
  public:
    chunk_t(uint32_t chunksize, uint32_t numchannels);
    ~chunk_t();
    uint32_t firstframe;
    uint32_t len;
    uint32_t channels;
    float* data;
  };

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
    uint32_t get_channels() const {return sf_inf.channels;};
  private:
    SNDFILE* sfile;
    SF_INFO sf_inf;
    uint32_t loopcnt_;
    uint32_t filepos_looped;
  };

  class async_sndfile_t {
  public:
    async_sndfile_t( uint32_t numchannels, uint32_t chunksize = (1 << 18) );
    async_sndfile_t( const async_sndfile_t& src );
    ~async_sndfile_t();
    /**
       \brief real-time safe, return audio data if available

       \param firstframe sample position of first frame
       \param n number of frames
       \param channels number of channels (must match number of channels given in constructor)
       \param buf pointer on data pointers, one pointer for each channel
     */
    void request_data( uint32_t firstframe, uint32_t n, uint32_t channels, float** buf );
    void open(const std::string& fname, uint32_t firstchannel, int32_t first_frame, double gain,uint32_t loop);
    void start_service();
    void stop_service();
  private:
    void service();
    static void * service(void* h);
    void rt_request_from_slave( uint32_t n );
    void slave_read_file( uint32_t firstframe );
    bool service_running;
    bool run_service;
    uint32_t numchannels_;
    uint32_t chunksize_;
    chunk_t ch1;
    chunk_t ch2;
    chunk_t* current;
    chunk_t* next;
    pthread_mutex_t mtx_readrequest;
    pthread_mutex_t mtx_next;
    pthread_mutex_t mtx_wakeup;
    // mtx_file protects sfile, file_channel and filebuffer:
    pthread_mutex_t mtx_file;
    uint32_t requested_startframe;
    bool need_data;
    looped_sndfile_t* sfile;
    uint32_t file_firstchannel;
    float* file_buffer;
    uint32_t file_pos;
    uint32_t file_frames;
    uint32_t file_channels;
    int32_t file_firstframe;
    int32_t file_lastframe;
    double gain_;
    //
    pthread_t srv_thread;
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
