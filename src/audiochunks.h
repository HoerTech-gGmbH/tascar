#ifndef AUDIOCHUNKS_H
#define AUDIOCHUNKS_H

#include <stdint.h>
#include <sndfile.h>
#include <string>

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
    uint32_t copy(float* data,uint32_t cnt,float gain=1.0);
    uint32_t copy_to(float* data,uint32_t cnt,float gain=1.0);
    void operator+=(const wave_t& o);
    float rms() const;
    void operator*=(double v);
    float* d;
    uint32_t n;
    bool own_pointer;
  };

  /** \brief Class for first-order-Ambisonics audio chunks
   */
  class amb1wave_t {
  public:
    amb1wave_t(uint32_t chunksize);
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
  protected:
    wave_t w_;
    wave_t x_;
    wave_t y_;
    wave_t z_;
  };

  class sndfile_handle_t {
  public:
    sndfile_handle_t(const std::string& fname);
    ~sndfile_handle_t();
    uint32_t get_frames() const {return sf_inf.frames;};
    uint32_t get_channels() const {return sf_inf.channels;};
    uint32_t readf_float( float* buf, uint32_t frames );
  private:
    SNDFILE* sfile;
    SF_INFO sf_inf;
  };

  class sndfile_t : public sndfile_handle_t, public wave_t {
  public:
    sndfile_t(const std::string& fname,uint32_t channel=0);
    void add_chunk(int32_t chunk_time, int32_t start_time,float gain,wave_t& chunk);
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
