/**
 * @file   audiochunks.h
 * @author Giso Grimm
 * @brief  Chunks for block-wise audio processing
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
#ifndef AUDIOCHUNKS_H
#define AUDIOCHUNKS_H

#include <sndfile.h>
#include "coordinates.h"

namespace TASCAR {

  /** \brief Class for single-channel time-domain audio chunks
   */
  class wave_t {
  public:
    wave_t(uint32_t chunksize);
    wave_t(const wave_t& src);
    wave_t(uint32_t n,float* ptr);
    ~wave_t();
    inline float& operator[](uint32_t k){return d[k];};
    inline const float& operator[](uint32_t k) const {return d[k];};
    inline uint32_t size() const { return n;};
    void clear();
    void copy(const wave_t& src,float gain=1.0);
    uint32_t copy(float* data,uint32_t cnt,float gain=1.0);
    uint32_t copy_to(float* data,uint32_t cnt,float gain=1.0);
    uint32_t copy_stride(float* data,uint32_t cnt,uint32_t stride,float gain=1.0);
    uint32_t copy_to_stride(float* data,uint32_t cnt,uint32_t stride,float gain=1.0);
    float ms() const;
    float rms() const;
    float maxabs() const;
    float spldb() const;
    float maxabsdb() const;
    void append(const wave_t& src);
    void operator*=(double v);
    void operator*=(float v);
    void operator+=(const wave_t& o);
    void operator*=(const wave_t& src);
    float* d;
    uint32_t n;
    bool own_pointer;
  protected:
    uint32_t append_pos;
    float rmsscale;
  };

  /** \brief Class for first-order-Ambisonics audio chunks
   */
  class amb1wave_t {
  public:
    amb1wave_t(uint32_t chunksize);
    amb1wave_t(uint32_t chunksize,float* pw,float* px,float* py,float* pz);
    inline wave_t& w(){return w_;};
    inline wave_t& x(){return x_;};
    inline wave_t& y(){return y_;};
    inline wave_t& z(){return z_;};
    inline const wave_t& w() const {return w_;};
    inline const wave_t& x() const {return x_;};
    inline const wave_t& y() const {return y_;};
    inline const wave_t& z() const {return z_;};
    inline uint32_t size() const { return w_.size();};
    void clear();
    void operator*=(double v);
    void operator+=(const amb1wave_t& v);
  protected:
    wave_t w_;
    wave_t x_;
    wave_t y_;
    wave_t z_;
  };

  class amb1rotator_t : public amb1wave_t {
  public:
    amb1rotator_t(uint32_t chunksize);
    amb1rotator_t& rotate(const amb1wave_t& src,const TASCAR::zyx_euler_t& o,bool invert=false);
  private:
    double wxx, wxy, wxz, wyx, wyy, wyz, wzx, wzy, wzz, dt;
  };

  class sndfile_handle_t {
  public:
    sndfile_handle_t(const std::string& fname);
    sndfile_handle_t(const std::string& fname, int samplerate, int channels);
    ~sndfile_handle_t();
    uint32_t get_channels() const {return sf_inf.channels;};
    uint32_t get_frames() const {return sf_inf.frames;};
    uint32_t get_srate() const {return sf_inf.samplerate;};
    uint32_t readf_float( float* buf, uint32_t frames );
    uint32_t writef_float( float* buf, uint32_t frames );
    static SF_INFO sf_info_configurator(int samplerate, int channels);
  private:
    SF_INFO sf_inf;
    SNDFILE* sfile;
  };

  class sndfile_t : public sndfile_handle_t, public wave_t {
  public:
    sndfile_t(const std::string& fname,uint32_t channel=0,double start=0,double length=0);
    void set_position(double position);
    void set_iposition(int64_t position);
    void set_loop(uint32_t loop);
    void add_to_chunk(int64_t chunktime,wave_t& chunk);
    void add_chunk(int32_t chunk_time, int32_t start_time,float gain,wave_t& chunk);
    void add_chunk_looped(float gain,wave_t& chunk);
  private:
    uint32_t looped_t;
    float looped_gain;
    // position parameters:
    int64_t iposition_;
    uint32_t loop_;
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
