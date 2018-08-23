/**
 * @file   async_file.h
 * @author Giso Grimm
 * 
 * @brief  Asynchronous playback of sound files via JACK
 */
/* 
 * License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
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
#ifndef ASYNC_FILE_H
#define ASYNC_FILE_H

#include "ringbuffer.h"
#include <string>
#include <sndfile.h>

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
    uint32_t get_channels() const {return sf_inf.channels;};
    uint32_t get_frames() const {return sf_inf.frames;};
    uint32_t get_loopedframes() const;
    uint32_t get_srate() const {return sf_inf.samplerate;};
  protected:
    std::string efname;
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
    uint32_t get_srate() const;
  private:
    void service();
    static void * service(void* h);
    bool service_running;
    bool run_service;
    uint32_t numchannels_;
    uint32_t buffer_length_;
    uint32_t fragsize_;
    ringbuffer_t rb;
    // mtx_file protects sfile, file_channel and filebuffer:
    pthread_mutex_t mtx_file;
    inftime_looped_sndfile_t* sfile;
    uint32_t file_firstchannel;
    float* file_buffer;
    float* read_fragment_buf;
    float* disk_fragment_buf;
    uint32_t file_channels;
    int32_t file_firstframe;
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
