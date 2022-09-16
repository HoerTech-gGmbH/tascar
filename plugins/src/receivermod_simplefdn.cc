/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "amb33defs.h"
#include "errorhandling.h"
#include "receivermod.h"

#include "fft.h"
#include <limits>
#include "optim.h"

const std::complex<double> i_d(0.0, 1.0);
const std::complex<float> i_f(0.0f, 1.0f);

/**
 * A First Order Ambisonics audio sample
 */
class foa_sample_t {
public:
  foa_sample_t() : w(0), x(0), y(0), z(0){};
  foa_sample_t(float w_, float x_, float y_, float z_)
      : w(w_), x(x_), y(y_), z(z_){};
  float w;
  float x;
  float y;
  float z;
  void clear()
  {
    w = 0.0;
    x = 0.0;
    y = 0.0;
    z = 0.0;
  };
  inline foa_sample_t& operator+=(const foa_sample_t& b)
  {
    w += b.w;
    x += b.x;
    y += b.y;
    z += b.z;
    return *this;
  }
  inline foa_sample_t& operator-=(const foa_sample_t& b)
  {
    w -= b.w;
    x -= b.x;
    y -= b.y;
    z -= b.z;
    return *this;
  }
  inline foa_sample_t& operator*=(float b)
  {
    w *= b;
    x *= b;
    y *= b;
    z *= b;
    return *this;
  };
};

inline foa_sample_t operator*(foa_sample_t self, float d)
{
  self *= d;
  return self;
};

inline foa_sample_t operator*(float d, foa_sample_t self)
{
  self *= d;
  return self;
};

inline foa_sample_t operator-(foa_sample_t a, const foa_sample_t& b)
{
  a -= b;
  return a;
}

inline foa_sample_t operator+(foa_sample_t a, const foa_sample_t& b)
{
  a += b;
  return a;
}

/**
 * Two-dimensional container for foa_sample_t
 */
class foa_sample_matrix_2d_t {
public:
  foa_sample_matrix_2d_t(uint32_t d1, uint32_t d2);
  ~foa_sample_matrix_2d_t();
  inline foa_sample_t& elem(uint32_t p1, uint32_t p2)
  {
    return data[p1 * s2 + p2];
  };
  inline const foa_sample_t& elem(uint32_t p1, uint32_t p2) const
  {
    return data[p1 * s2 + p2];
  };
  inline foa_sample_t& elem00x(uint32_t p2) { return data[p2]; };
  inline const foa_sample_t& elem00x(uint32_t p2) const { return data[p2]; };
  inline void clear()
  {
    uint32_t l(s1 * s2);
    for(uint32_t k = 0; k < l; ++k)
      data[k].clear();
  };

protected:
  uint32_t s1;
  uint32_t s2;
  foa_sample_t* data;
};

class foa_sample_array_1d_t {
public:
  foa_sample_array_1d_t(uint32_t d1);
  ~foa_sample_array_1d_t();
  inline foa_sample_t& elem(uint32_t p1) { return data[p1]; };
  inline const foa_sample_t& elem(uint32_t p1) const { return data[p1]; };
  inline void clear()
  {
    for(uint32_t k = 0; k < s1; ++k)
      data[k].clear();
  };

protected:
  uint32_t s1;
  foa_sample_t* data;
};

foa_sample_matrix_2d_t::foa_sample_matrix_2d_t(uint32_t d1, uint32_t d2)
    : s1(d1), s2(d2), data(new foa_sample_t[s1 * s2])
{
  clear();
}

foa_sample_matrix_2d_t::~foa_sample_matrix_2d_t()
{
  delete[] data;
}

foa_sample_array_1d_t::foa_sample_array_1d_t(uint32_t d1)
    : s1(d1), data(new foa_sample_t[s1])
{
  clear();
}

foa_sample_array_1d_t::~foa_sample_array_1d_t()
{
  delete[] data;
}

// y[n] = -g x[n] + x[n-1] + g y[n-1]
class reflectionfilter_t {
public:
  reflectionfilter_t(uint32_t d1);
  inline void filter(foa_sample_t& x, uint32_t p1)
  {
    x *= B1;
    x -= A2 * sy.elem(p1);
    sy.elem(p1) = x;
    // all pass section:
    foa_sample_t tmp(eta[p1] * x + sapx.elem(p1));
    sapx.elem(p1) = x;
    x = tmp - eta[p1] * sapy.elem(p1);
    sapy.elem(p1) = x;
  };
  void set_lp(float g, float c);

protected:
  float B1; ///< non-recursive filter coefficient for all channels
  float A2; ///< recursive filter coefficient for all channels
  std::vector<float>
      eta; ///< phase coefficient of allpass filters for each channel
  foa_sample_array_1d_t sy;   ///< output state buffer
  foa_sample_array_1d_t sapx; ///< input state variable of allpass filter
  foa_sample_array_1d_t sapy; ///< output state variable of allpass filter
};

reflectionfilter_t::reflectionfilter_t(uint32_t d1)
    : B1(0), A2(0), sy(d1), sapx(d1), sapy(d1)
{
  eta.resize(d1);
  for(uint32_t k = 0; k < d1; ++k)
    eta[k] = 0.87f * (float)k / ((float)d1 - 1.0f);
}

/**
 * Set low pass filter coefficient and gain of reflection filter,
 * leave allpass coefficient untouched.
 *
 * @param g Scaling factor
 * @param c Recursive lowpass filter coefficient
 */
void reflectionfilter_t::set_lp(float g, float c)
{
  sy.clear();
  sapx.clear();
  sapy.clear();
  float c2(1.0f - c);
  B1 = g * c2;
  A2 = -c;
}

class fdn_t {
public:
  enum gainmethod_t { original, mean, schroeder };
  fdn_t(uint32_t fdnorder, uint32_t maxdelay, bool logdelays, gainmethod_t gm);
  ~fdn_t();
  inline void process(bool b_prefilt);
  void setpar_t60(float az, float daz, float t, float dt, float t60,
                  float damping, bool fixcirculantmat);
  void set_logdelays(bool ld) { logdelays_ = ld; };
  void clear()
  {
    delayline.clear();
    outval.clear();
  };
  void get_ir(TASCAR::wave_t& w, bool b_prefilt);

private:
  bool logdelays_;
  uint32_t fdnorder_;
  uint32_t maxdelay_;
  // delayline:
  foa_sample_matrix_2d_t delayline;
  // feedback matrix:
  std::vector<float> feedbackmat;
  // reflection filter:
  reflectionfilter_t reflection;
  reflectionfilter_t prefilt;
  // rotation:
  std::vector<TASCAR::quaternion_t> rotation;
  // delayline output for reflection filters:
  foa_sample_array_1d_t dlout;
  // delays:
  uint32_t* delay;
  // delayline pointer:
  uint32_t* pos;
  // gain calculation method:
  gainmethod_t gainmethod;

public:
  // input FOA sample:
  foa_sample_t inval;
  // output FOA sample:
  foa_sample_t outval;
};

fdn_t::fdn_t(uint32_t fdnorder, uint32_t maxdelay, bool logdelays,
             gainmethod_t gm)
    : logdelays_(logdelays), fdnorder_(fdnorder), maxdelay_(maxdelay),
      delayline(fdnorder_, maxdelay_), feedbackmat(fdnorder_ * fdnorder_),
      reflection(fdnorder), prefilt(2), rotation(fdnorder), dlout(fdnorder_),
      delay(new uint32_t[fdnorder_]), pos(new uint32_t[fdnorder_]),
      gainmethod(gm)
{
  memset(delay, 0, sizeof(uint32_t) * fdnorder_);
  memset(pos, 0, sizeof(uint32_t) * fdnorder_);
}

fdn_t::~fdn_t()
{
  delete[] delay;
  delete[] pos;
}

void fdn_t::process(bool b_prefilt)
{
  if(b_prefilt) {
    prefilt.filter(inval, 0);
    prefilt.filter(inval, 1);
  }
  outval.clear();
  // get output values from delayline, apply reflection filters and rotation:
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    foa_sample_t tmp(delayline.elem(tap, pos[tap]));
    reflection.filter(tmp, tap);
    TASCAR::posf_t p(tmp.x, tmp.y, tmp.z);
    rotation[tap].rotate(p);
    tmp.x = p.x;
    tmp.y = p.y;
    tmp.z = p.z;
    dlout.elem(tap) = tmp;
    outval += tmp;
  }
  // put rotated+attenuated value to delayline, add input:
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    // first put input into delayline:
    delayline.elem(tap, pos[tap]) = inval;
    // now add feedback signal:
    for(uint32_t otap = 0; otap < fdnorder_; ++otap)
      delayline.elem(tap, pos[tap]) +=
          dlout.elem(otap) * feedbackmat[fdnorder_ * tap + otap];
    // iterate delayline:
    if(!pos[tap])
      pos[tap] = delay[tap];
    if(pos[tap])
      --pos[tap];
  }
}

void fdn_t::get_ir(TASCAR::wave_t& ir, bool b_prefilt)
{
  clear();
  inval.w = 1.0f;
  inval.x = 1.0f;
  inval.y = 0.0f;
  inval.z = 0.0f;
  for(size_t k = 0u; k < ir.n; ++k) {
    process(b_prefilt);
    ir.d[k] = outval.w;
  }
  clear();
}

/**
   \brief Set parameters of FDN
   \param az Average rotation in radians per reflection
   \param daz Spread of rotation in radians per reflection
   \param t Average/maximum delay in samples
   \param dt Spread of delay in samples
   \param g Gain
   \param damping Damping
*/
void fdn_t::setpar_t60(float az, float daz, float t_min, float t_max, float t60,
                       float damping, bool fixcirculantmat)
{
  // set delays:
  delayline.clear();
  float t_mean(0);
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    float t_(t_min);
    if(logdelays_) {
      // logarithmic distribution:
      if(fdnorder_ > 1)
        t_ =
            t_min * powf(t_max / t_min, (float)tap / ((float)fdnorder_ - 1.0f));
      ;
    } else {
      // squareroot distribution:
      if(fdnorder_ > 1)
        t_ = t_min + (t_max - t_min) *
                         powf((float)tap / ((float)fdnorder_ - 1.0f), 0.5f);
    }
    uint32_t d((uint32_t)std::max(0.0f, t_));
    delay[tap] = std::max(2u, std::min(maxdelay_ - 1u, d));
    t_mean += (float)delay[tap];
  }
  t_mean /= (float)std::max(1u, fdnorder_);
  float g(0.0f);
  switch(gainmethod) {
  case fdn_t::original:
    g = expf(-4.2f * t_min / t60);
    break;
  case fdn_t::mean:
    g = expf(-4.2f * t_mean / t60);
    break;
  case fdn_t::schroeder:
    g = powf(10.0f, -3.0f * t_mean / t60);
    break;
  }
  // set reflection filters:
  reflection.set_lp(g, damping);
  prefilt.set_lp(g, damping);
  // set rotation:
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    float laz(az);
    if(fdnorder_ > 1)
      laz = az - daz + 2.0f * daz * (float)tap / (float)fdnorder_;
    rotation[tap].set_rotation(laz, TASCAR::posf_t(0, 0, 1));
    TASCAR::quaternion_t q;
    q.set_rotation(0.5f * daz * (float)(tap & 1) - 0.5f * daz,
                   TASCAR::posf_t(0, 1, 0));
    rotation[tap].rmul(q);
    q.set_rotation(0.125f * daz * (float)(tap % 3) - 0.25f * daz,
                   TASCAR::posf_t(1, 0, 0));
    rotation[tap].rmul(q);
  }
  // set feedback matrix:
  if(fdnorder_ > 1) {
    TASCAR::fft_t fft(fdnorder_);
    TASCAR::spec_t eigenv(fdnorder_ / 2 + 1);
    for(uint32_t k = 0; k < eigenv.n_; ++k)
      eigenv[k] = std::exp(i_f * TASCAR_2PIf *
                           powf((float)k / (0.5f * (float)fdnorder_), 2.0f));
    ;
    fft.execute(eigenv);
    for(uint32_t itap = 0; itap < fdnorder_; ++itap)
      for(uint32_t otap = 0; otap < fdnorder_; ++otap)
        if(fixcirculantmat)
          feedbackmat[fdnorder_ * itap + otap] =
              fft.w[(otap + fdnorder_ - itap) % fdnorder_];
        else
          feedbackmat[fdnorder_ * itap + otap] =
              fft.w[(otap + itap) % fdnorder_];
  } else {
    feedbackmat[0] = 1.0;
  }
}

class simplefdn_vars_t : public TASCAR::receivermod_base_t {
public:
  simplefdn_vars_t(tsccfg::node_t xmlsrc);
  ~simplefdn_vars_t();
  // protected:
  uint32_t fdnorder = 5;
  float w = 0.0;
  float dw = 60.0;
  float t60 = 0.0;
  float damping = 0.3f;
  bool prefilt = true;
  bool logdelays = true;
  float absorption = 0.6f;
  float c = 340.0f;
  bool fixcirculantmat = false;
  TASCAR::pos_t volumetric;
  fdn_t::gainmethod_t gm;
  std::vector<float> vcf;
  std::vector<float> vt60;
  uint32_t numiter = 100;
};

simplefdn_vars_t::simplefdn_vars_t(tsccfg::node_t xmlsrc)
    : receivermod_base_t(xmlsrc)
{
  GET_ATTRIBUTE(fdnorder, "", "Order of FDN (number of recursive paths)");
  GET_ATTRIBUTE(dw, "rounds/s", "Spatial spread of rotation");
  GET_ATTRIBUTE(t60, "s", "T60, or zero to use Sabine's equation");
  GET_ATTRIBUTE(damping, "",
                "Damping (first order lowpass) coefficient to control spectral "
                "tilt of T60");
  GET_ATTRIBUTE_BOOL(prefilt,
                     "Apply additional filter before inserting audio into FDN");
  GET_ATTRIBUTE(absorption, "", "Absorption used in Sabine's equation");
  GET_ATTRIBUTE(c, "m/s", "Speed of sound");
  GET_ATTRIBUTE(volumetric, "m", "Dimension of room x y z");
  GET_ATTRIBUTE_BOOL(
      fixcirculantmat,
      "Apply fix to correctly initialize circulant feedback matrix");
  GET_ATTRIBUTE(
      vcf, "Hz",
      "Center frequencies for T60 optimization, or empty for no optimization");
  GET_ATTRIBUTE(vt60, "s", "T60 at specified center frequencies");
  if(vcf.size() != vt60.size())
    throw TASCAR::ErrMsg("Mismatching number of entries in vcf and vt60.");
  GET_ATTRIBUTE(numiter, "", "Number of iterations in T60 optimization");
  std::string gainmethod("original");
  GET_ATTRIBUTE(gainmethod, "original mean schroeder",
                "Gain calculation method");
  if(gainmethod == "original")
    gm = fdn_t::original;
  else if(gainmethod == "mean")
    gm = fdn_t::mean;
  else if(gainmethod == "schroeder")
    gm = fdn_t::schroeder;
  else
    throw TASCAR::ErrMsg(
        "Invalid gain method \"" + gainmethod +
        "\". Possible values are original, mean or schroeder.");
}

simplefdn_vars_t::~simplefdn_vars_t() {}

class simplefdn_t : public simplefdn_vars_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB11ACN::idx::channels];
    float w_current[AMB11ACN::idx::channels];
    float dw[AMB11ACN::idx::channels];
    float dt;
  };
  simplefdn_t(tsccfg::node_t xmlsrc);
  ~simplefdn_t();
  void postproc(std::vector<TASCAR::wave_t>& output);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t&,
                               std::vector<TASCAR::wave_t>&,
                               receivermod_base_t::data_t*){};
  void update_par();
  void setlogdelays(bool ld);
  void configure();
  void optim_param();
  void release();
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  virtual void add_variables(TASCAR::osc_server_t* srv);
  static int osc_set_dim_damp_absorption(const char* path, const char* types,
                                         lo_arg** argv, int argc,
                                         lo_message msg, void* user_data);
  static int osc_fixcirculantmat(const char* path, const char* types,
                                 lo_arg** argv, int argc, lo_message msg,
                                 void* user_data);
  /**
     @brief Get T60 at octave bands around provided center frequencies
     @param cf list of center frequencies in Hz
     @retval t60 T60 in seconds
   */
  void get_t60(const std::vector<float>& cf, std::vector<float>& t60);
  float t60err(const std::vector<float>& param);
  float slopeerr(const std::vector<float>& param);

private:
  fdn_t* fdn;
  TASCAR::amb1wave_t* foa_out;
  pthread_mutex_t mtx;
  float wgain;
  float distcorr;
  TASCAR::wave_t* ir_bb;
  TASCAR::wave_t* ir_band;
};

int simplefdn_t::osc_fixcirculantmat(const char*, const char* types,
                                     lo_arg** argv, int argc, lo_message,
                                     void* user_data)
{
  if((1 == argc) && (types[0] == 'i')) {
    ((simplefdn_t*)user_data)->fixcirculantmat = (argv[0]->i > 0);
    ((simplefdn_t*)user_data)->update_par();
  }
  return 0;
}

int simplefdn_t::osc_set_dim_damp_absorption(const char*, const char* types,
                                             lo_arg** argv, int argc,
                                             lo_message, void* user_data)
{
  if((5 == argc) && (types[0] == 'f') && (types[1] == 'f') &&
     (types[2] == 'f') && (types[3] == 'f') && (types[4] == 'f')) {
    ((simplefdn_t*)user_data)->volumetric.x = argv[0]->f;
    ((simplefdn_t*)user_data)->volumetric.y = argv[1]->f;
    ((simplefdn_t*)user_data)->volumetric.z = argv[2]->f;
    ((simplefdn_t*)user_data)->damping = argv[3]->f;
    ((simplefdn_t*)user_data)->absorption = argv[4]->f;
    ((simplefdn_t*)user_data)->t60 = 0.0;
    ((simplefdn_t*)user_data)->update_par();
  }
  return 0;
}

simplefdn_t::simplefdn_t(tsccfg::node_t cfg)
    : simplefdn_vars_t(cfg), fdn(NULL), foa_out(NULL), wgain(MIN3DB),
      distcorr(1.0)
{
  if(t60 <= 0.0)
    t60 =
        0.161f * volumetric.boxvolumef() / (absorption * volumetric.boxareaf());
  pthread_mutex_init(&mtx, NULL);
}

static float t60err_(const std::vector<float>& param, void* data)
{
  return ((simplefdn_t*)data)->t60err(param);
}

static float slopeerr_(const std::vector<float>& param, void* data)
{
  return ((simplefdn_t*)data)->slopeerr(param);
}

float simplefdn_t::t60err(const std::vector<float>& param)
{
  if(param.empty())
    throw TASCAR::ErrMsg("Invalid (empty) parameter space");
  absorption = std::max(0.0f, std::min(1.0f, param[0]));
  t60 = 0.0f;
  update_par();
  std::vector<float> xt60;
  get_t60(vcf, xt60);
  float t60max = 0.0f;
  float t60max_ref = 0.0f;
  for(size_t k = 0; k < std::min(xt60.size(), vt60.size()); ++k) {
    t60max = std::max(t60max, xt60[k]);
    t60max_ref = std::max(t60max_ref, vt60[k]);
  }
  float le = fabs(t60max / t60max_ref - 1.0f);
  le *= le;
  return le;
}

float simplefdn_t::slopeerr(const std::vector<float>& param)
{
  if(param.empty())
    throw TASCAR::ErrMsg("Invalid (empty) parameter space");
  damping = std::max(0.0f, std::min(0.999f, param[0]));
  update_par();
  std::vector<float> xt60;
  get_t60(vcf, xt60);
  float slope = 0.0f;
  for(size_t k = 1; k < std::min(xt60.size(), vt60.size()); ++k)
    slope += (xt60[k] - xt60[0]) / (logf(vcf[k]) - logf(vcf[0]));
  float slope_ref = 0.0f;
  for(size_t k = 1; k < std::min(xt60.size(), vt60.size()); ++k)
    slope_ref += (vt60[k] - vt60[0]) / (logf(vcf[k]) - logf(vcf[0]));
  float le = fabs(slope / slope_ref - 1.0f);
  le *= le;
  return le;
}

void simplefdn_t::configure()
{
  receivermod_base_t::configure();
  n_channels = AMB11ACN::idx::channels;
  if(fdn)
    delete fdn;
  fdn = new fdn_t(fdnorder, (uint32_t)f_sample, logdelays, gm);
  if(foa_out)
    delete foa_out;
  foa_out = new TASCAR::amb1wave_t(n_fragment);
  labels.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    char ctmp[32];
    sprintf(ctmp, ".%d%c", (ch > 0), AMB11ACN::channelorder[ch]);
    labels.push_back(ctmp);
  }
  // prepare impulse response container for T60 measurements:
  size_t irlen((size_t)f_sample);
  float maxdim((float)volumetric.x);
  maxdim = std::max(maxdim, (float)volumetric.y);
  maxdim = std::max(maxdim, (float)volumetric.z);
  size_t irlendim((size_t)(10.0f * maxdim / c * (float)f_sample));
  irlen = std::max(irlen, irlendim);
  ir_bb = new TASCAR::wave_t((uint32_t)irlen);
  ir_band = new TASCAR::wave_t((uint32_t)irlen);
  if(vcf.size() > 0) {
    // optimize damping and absorption to match given T60
    // first optimize absorption
    t60 = 0.0f;
    damping = 0.2f;
    absorption =
        0.161f * volumetric.boxvolumef() / (vt60[0] * volumetric.boxareaf());
    float eps = 1.0f;
    float lasterr = 10000.0f;
    std::vector<float> param = {absorption};
    for(size_t it = 0; it < numiter; ++it) {
      float err = downhill_iterate(eps, param, t60err_, this, {1e-3f});
      param[0] = fabs(param[0]);
      if((err < 0.00005) || (fabsf(lasterr / err - 1.0f) < 1e-9f))
        it = numiter;
      lasterr = err;
    }
    absorption = fabsf(param[0]);
    param = {(float)damping};
    lasterr = 10000.0f;
    for(size_t it = 0; it < numiter; ++it) {
      float err = downhill_iterate(eps, param, slopeerr_, this, {2e-3f});
      param[0] = std::min(0.99f, fabs(param[0]));
      if(err > 4.0f * lasterr)
        eps *= 0.5f;
      if((err < 0.00005f) || (fabsf(lasterr / err - 1.0f) < 1e-9f))
        it = numiter;
      lasterr = err;
    }
    damping = param[0];
    t60 = 0.0f;
    update_par();
    std::vector<float> xt60;
    get_t60(vcf, xt60);
    std::cout << "Optimization of T60:\n  absorption=\"" << absorption
              << "\" damping=\"" << damping << "\" t60=\"0\"\n";
    for(size_t k = 0; k < vcf.size(); ++k) {
      std::cout << "  " << vcf[k] << " Hz: " << xt60[k] << " s\n";
    }
    std::cout << "  (T60_sab = " << t60 << "s)\n";
    t60 = 0.0f;
  }
  update_par();
}

void simplefdn_t::release()
{
  TASCAR::receivermod_base_t::release();
  delete ir_bb;
  delete ir_band;
}

void simplefdn_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->add_method("/fixcirculantmat", "i", &osc_fixcirculantmat, this);
  srv->add_method("/dim_damp_absorption", "fffff", &osc_set_dim_damp_absorption,
                  this);
}

simplefdn_t::~simplefdn_t()
{
  if(fdn)
    delete fdn;
  if(foa_out)
    delete foa_out;
  pthread_mutex_destroy(&mtx);
}

void simplefdn_t::update_par()
{
  if(pthread_mutex_lock(&mtx) == 0) {
    distcorr = 1.0f / (0.5f * powf((float)volumetric.x * (float)volumetric.y *
                                       (float)volumetric.z,
                                   0.33333f));
    float tmin(std::min((float)volumetric.x,
                        std::min((float)volumetric.y, (float)volumetric.z)) /
               c);
    float tmax(std::max((float)volumetric.x,
                        std::max((float)volumetric.y, (float)volumetric.z)) /
               c);
    if(t60 <= 0.0)
      t60 = 0.161f * volumetric.boxvolumef() /
            (absorption * volumetric.boxareaf());
    if(fdn) {
      float wscale(TASCAR_2PIf * tmin);
      fdn->setpar_t60(wscale * w, wscale * dw, (float)f_sample * tmin,
                      (float)f_sample * tmax, (float)f_sample * t60,
                      std::max(0.0f, std::min(0.999f, damping)),
                      fixcirculantmat);
    }
    pthread_mutex_unlock(&mtx);
  }
}

void simplefdn_t::setlogdelays(bool ld)
{
  if(pthread_mutex_lock(&mtx) == 0) {
    if(fdn) {
      fdn->set_logdelays(ld);
      float tmin(std::min((float)volumetric.x,
                          std::min((float)volumetric.y, (float)volumetric.z)) /
                 c);
      float tmax(std::max((float)volumetric.x,
                          std::max((float)volumetric.y, (float)volumetric.z)) /
                 c);
      if(t60 <= 0.0f)
        t60 = 0.161f * volumetric.boxvolumef() /
              (absorption * volumetric.boxareaf());
      float wscale(TASCAR_2PIf * tmin);
      fdn->setpar_t60(wscale * w, wscale * dw, (float)f_sample * tmin,
                      (float)f_sample * tmax, (float)f_sample * t60,
                      std::max(0.0f, std::min(0.999f, damping)),
                      fixcirculantmat);
    }
    pthread_mutex_unlock(&mtx);
  }
}

void simplefdn_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  // correct for volumetric gain:
  if(pthread_mutex_trylock(&mtx) == 0) {
    *foa_out *= distcorr;
    if(fdn) {
      for(uint32_t t = 0; t < n_fragment; ++t) {
        fdn->inval.w = foa_out->w()[t];
        fdn->inval.x = foa_out->x()[t];
        fdn->inval.y = foa_out->y()[t];
        fdn->inval.z = foa_out->z()[t];
        fdn->process(prefilt);
        output[AMB11ACN::idx::w][t] += fdn->outval.w;
        output[AMB11ACN::idx::x][t] += fdn->outval.x;
        output[AMB11ACN::idx::y][t] += fdn->outval.y;
        output[AMB11ACN::idx::z][t] += fdn->outval.z;
      }
    }
    foa_out->clear();
    pthread_mutex_unlock(&mtx);
  }
}

float ir_get_t60(TASCAR::wave_t& ir, float fs)
{
  if(ir.n < 2)
    return -1;
  // max threshold is -0.2 dB below max:
  float thmax = powf(10.0f, -0.1f * 10.2f);
  float thmin = powf(10.0f, -0.1f * 30.2f);
  float cs = 0.0f;
  for(size_t k = ir.n - 1; k != 0; --k) {
    cs += ir.d[k] * ir.d[k];
    ir.d[k] = cs;
  }
  thmax *= cs;
  thmin *= cs;
  size_t idx_max = 0;
  size_t idx_min = 0;
  for(size_t k = 0; k < ir.n; ++k) {
    if(ir.d[k] > thmax)
      idx_max = k;
    if(ir.d[k] > thmin)
      idx_min = k;
  }
  if(idx_min > idx_max) {
    cs = (-60.0f / (10.0f * log10f(ir.d[idx_min] / ir.d[idx_max]) * fs)) *
         (float)(idx_min - idx_max);
    return cs;
  }
  return -1.0f;
}

void simplefdn_t::get_t60(const std::vector<float>& cf, std::vector<float>& t60)
{
  if(pthread_mutex_trylock(&mtx) == 0) {
    if(fdn) {
      t60.clear();
      fdn->get_ir(*ir_bb, prefilt);
      TASCAR::bandpass_t bp(176.78f, 353.55f, f_sample);
      for(auto f : cf) {
        bp.set_range(f * sqrtf(0.5f), f * sqrtf(2.0f));
        ir_band->copy(*ir_bb);
        bp.filter(*ir_band);
        bp.clear();
        bp.filter(*ir_band);
        bp.clear();
        bp.filter(*ir_band);
        bp.clear();
        bp.filter(*ir_band);
        t60.push_back(ir_get_t60(*ir_band, (float)f_sample));
      }
    }
    pthread_mutex_unlock(&mtx);
  }
}

TASCAR::receivermod_base_t::data_t*
simplefdn_t::create_state_data(double, uint32_t fragsize) const
{
  return new data_t(fragsize);
}

void simplefdn_t::add_pointsource(const TASCAR::pos_t& prel, double,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>&,
                                  receivermod_base_t::data_t* sd)
{
  TASCAR::pos_t pnorm(prel.normal());
  data_t* d((data_t*)sd);
  // use ACN everywhere:
  d->_w[AMB11ACN::idx::w] = wgain;
  d->_w[AMB11ACN::idx::x] = (float)pnorm.x;
  d->_w[AMB11ACN::idx::y] = (float)pnorm.y;
  d->_w[AMB11ACN::idx::z] = (float)pnorm.z;
  for(uint32_t acn = 0; acn < AMB11ACN::idx::channels; ++acn)
    d->dw[acn] = (d->_w[acn] - d->w_current[acn]) * d->dt;
  for(uint32_t acn = 0; acn < AMB11ACN::idx::channels; ++acn)
    for(uint32_t i = 0; i < chunk.size(); i++)
      (*foa_out)[acn][i] += (d->w_current[acn] += d->dw[acn]) * chunk[i];
  for(uint32_t acn = 0; acn < AMB11ACN::idx::channels; ++acn)
    d->w_current[acn] = d->_w[acn];
}

simplefdn_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k = 0; k < AMB11ACN::idx::channels; k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0f / std::max(1.0f, (float)chunksize);
}

REGISTER_RECEIVERMOD(simplefdn_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
