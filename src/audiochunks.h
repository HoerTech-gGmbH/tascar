#ifndef AUDIOCHUNKS_H
#define AUDIOCHUNKS_H

#include <stdint.h>

namespace TASCAR {

  /** \brief Class for single-channel time-domain audio chunks
   */
  class wave_t {
  public:
    wave_t(uint32_t chunksize);
    wave_t(const wave_t& src);
    ~wave_t();
    inline float& operator[](uint32_t k){return d[k];};
    inline const float& operator[](uint32_t k) const {return d[k];};
    inline uint32_t size() const { return n;};
    void clear();
    uint32_t copy(float* data,uint32_t cnt,float gain=1.0);
    uint32_t copy_to(float* data,uint32_t cnt,float gain=1.0);
    void operator+=(const wave_t& o);
    float rms() const;
  protected:
    float* d;
    uint32_t n;
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
  protected:
    wave_t w_;
    wave_t x_;
    wave_t y_;
    wave_t z_;
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
