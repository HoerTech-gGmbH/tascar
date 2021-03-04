#include "amb33defs.h"
#include "errorhandling.h"
#include "receivermod.h"

#include "fft.h"
#include <limits>

const std::complex<float> i(0.0, 1.0);
const std::complex<double> i_d(0.0, 1.0);

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

class cmat3_t {
public:
  cmat3_t(uint32_t d1, uint32_t d2);
  ~cmat3_t();
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

class cmat2_t {
public:
  cmat2_t(uint32_t d1);
  ~cmat2_t();
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

cmat3_t::cmat3_t(uint32_t d1, uint32_t d2)
    : s1(d1), s2(d2), data(new foa_sample_t[s1 * s2])
{
  clear();
}

cmat3_t::~cmat3_t()
{
  delete[] data;
}

cmat2_t::cmat2_t(uint32_t d1) : s1(d1), data(new foa_sample_t[s1])
{
  clear();
}

cmat2_t::~cmat2_t()
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
  float B1;
  float A2;
  std::vector<float> eta;
  cmat2_t sy;
  cmat2_t sapx;
  cmat2_t sapy;
};

reflectionfilter_t::reflectionfilter_t(uint32_t d1)
    : B1(0), A2(0), sy(d1), sapx(d1), sapy(d1)
{
  eta.resize(d1);
  for(uint32_t k = 0; k < d1; ++k)
    eta[k] = 0.87 * (double)k / (d1 - 1);
}

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
  fdn_t(uint32_t fdnorder, uint32_t maxdelay, bool logdelays);
  ~fdn_t();
  inline void process(bool b_prefilt);
  void setpar(float az, float daz, float t, float dt, float g, float damping);
  void set_logdelays(bool ld) { logdelays_ = ld; };

private:
  bool logdelays_;
  uint32_t fdnorder_;
  uint32_t maxdelay_;
  uint32_t taplen;
  // delayline:
  cmat3_t delayline;
  // feedback matrix:
  std::vector<float> feedbackmat;
  // reflection filter:
  reflectionfilter_t reflection;
  reflectionfilter_t prefilt;
  // rotation:
  std::vector<TASCAR::quaternion_t> rotation;
  // delayline output for reflection filters:
  cmat2_t dlout;
  // delays:
  uint32_t* delay;
  // delayline pointer:
  uint32_t* pos;

public:
  // input FOA sample:
  foa_sample_t inval;
  // output FOA sample:
  foa_sample_t outval;
};

fdn_t::fdn_t(uint32_t fdnorder, uint32_t maxdelay, bool logdelays)
    : logdelays_(logdelays), fdnorder_(fdnorder), maxdelay_(maxdelay),
      taplen(maxdelay * 2), delayline(fdnorder_, maxdelay_),
      feedbackmat(fdnorder_ * fdnorder_), reflection(fdnorder), prefilt(2),
      rotation(fdnorder), dlout(fdnorder_), delay(new uint32_t[fdnorder_]),
      pos(new uint32_t[fdnorder_])
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
    TASCAR::pos_t p(tmp.x, tmp.y, tmp.z);
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

/**
   \brief Set parameters of FDN
   \param az Average rotation in radians per reflection
   \param daz Spread of rotation in radians per reflection
   \param t Average/maximum delay in samples
   \param dt Spread of delay in samples
   \param g Gain
   \param damping Damping
*/
void fdn_t::setpar(float az, float daz, float t, float dt, float g,
                   float damping)
{
  // set reflection filters:
  reflection.set_lp(g, damping);
  prefilt.set_lp(g, damping);
  // reflection.set( g );
  // set delays:
  delayline.clear();
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    double t_(t);
    if(logdelays_) {
      // logarithmic distribution:
      if(fdnorder_ > 1)
        t_ = t * pow((t + dt) / t, (double)tap / ((double)fdnorder_ - 1.0));
      ;
    } else {
      // squareroot distribution:
      if(fdnorder_ > 1)
        t_ = t + dt * pow((double)tap / ((double)fdnorder_ - 1.0), 0.5);
    }
    uint32_t d(std::max(0.0, t_));
    delay[tap] = std::max(2u, std::min(maxdelay_ - 1u, d));
  }
  // set rotation:
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    float laz(az);
    if(fdnorder_ > 1)
      laz = az - daz + 2.0 * daz * tap * 1.0 / (fdnorder_);
    std::complex<float> caz(std::exp(i * laz));
    rotation[tap].set_rotation(laz, TASCAR::pos_t(0, 0, 1));
    TASCAR::quaternion_t q;
    q.set_rotation(0.5 * daz * (tap & 1) - 0.5 * daz, TASCAR::pos_t(0, 1, 0));
    rotation[tap] *= q;
    q.set_rotation(0.125 * daz * (tap % 3) - 0.25 * daz,
                   TASCAR::pos_t(1, 0, 0));
    rotation[tap] *= q;
  }
  // set feedback matrix:
  if(fdnorder_ > 1) {
    TASCAR::fft_t fft(fdnorder_);
    TASCAR::spec_t eigenv(fdnorder_ / 2 + 1);
    for(uint32_t k = 0; k < eigenv.n_; ++k)
      eigenv[k] =
          std::exp(i_d * 2.0 * M_PI * pow((double)k / (0.5 * fdnorder_), 2.0));
    ;
    fft.execute(eigenv);
    // std::cout << "row: " << fft.w << std::endl;
    for(uint32_t itap = 0; itap < fdnorder_; ++itap) {
      for(uint32_t otap = 0; otap < fdnorder_; ++otap) {
        feedbackmat[fdnorder_ * itap + otap] = fft.w[(otap + itap) % fdnorder_];
      }
    }
  } else {
    feedbackmat[0] = 1.0;
  }
}

class simplefdn_vars_t : public TASCAR::receivermod_base_t {
public:
  simplefdn_vars_t(xmlpp::Element* xmlsrc);
  ~simplefdn_vars_t();
  // protected:
  uint32_t fdnorder;
  double w;
  double dw;
  double t;
  double dt;
  double t60;
  double damping;
  bool prefilt;
  bool logdelays;
  double absorption;
  double c;
  TASCAR::pos_t volumetric;
};

simplefdn_vars_t::simplefdn_vars_t(xmlpp::Element* xmlsrc)
    : receivermod_base_t(xmlsrc), fdnorder(5), w(0.0), dw(60.0), t(0.01),
      dt(0.002), t60(0), damping(0.3), prefilt(true), logdelays(true),
      absorption(0.6), c(340)
{
  GET_ATTRIBUTE(fdnorder,"","Order of FDN");
  GET_ATTRIBUTE(dw,"rad/s","Spatial spread");
  GET_ATTRIBUTE(t60,"s","$T_{60}$, or zero to use Sabine's equation");
  GET_ATTRIBUTE(damping,"","Damping (first order lowpass) coefficient");
  GET_ATTRIBUTE_BOOL(prefilt,"Filter before feedback matrix");
  GET_ATTRIBUTE(absorption,"","Absorption used in Sabine's equation");
  GET_ATTRIBUTE(c,"m/s","Speed of sound");
  GET_ATTRIBUTE(volumetric,"m","Dimension of room x y z");
}

simplefdn_vars_t::~simplefdn_vars_t() {}

class simplefdn_t : public simplefdn_vars_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB11::idx::channels];
    float w_current[AMB11::idx::channels];
    float dw[AMB11::idx::channels];
    double dt;
  };
  simplefdn_t(xmlpp::Element* xmlsrc);
  ~simplefdn_t();
  void postproc(std::vector<TASCAR::wave_t>& output);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*){};
  void update_par();
  void setlogdelays(bool ld);
  void configure();
  receivermod_base_t::data_t* create_data(double srate, uint32_t fragsize);
  virtual void add_variables(TASCAR::osc_server_t* srv);
  static int osc_set_dim_damp_absorption(const char* path, const char* types,
                                         lo_arg** argv, int argc,
                                         lo_message msg, void* user_data);

private:
  uint32_t channels;
  fdn_t* fdn;
  TASCAR::amb1wave_t* foa_out;
  pthread_mutex_t mtx;
  float wgain;
  float distcorr;
};

int simplefdn_t::osc_set_dim_damp_absorption(const char* path,
                                             const char* types, lo_arg** argv,
                                             int argc, lo_message msg,
                                             void* user_data)
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

simplefdn_t::simplefdn_t(xmlpp::Element* cfg)
    : simplefdn_vars_t(cfg), fdn(NULL), foa_out(NULL), wgain(MIN3DB),
      distcorr(1.0)
{
  double dmin(std::min(volumetric.x, std::min(volumetric.y, volumetric.z)) / c);
  double dmax(std::max(volumetric.x, std::max(volumetric.y, volumetric.z)) / c);
  t = dmin;
  dt = (dmax - dmin);
  if(t60 <= 0.0)
    t60 = 0.161 * volumetric.boxvolume() / (absorption * volumetric.boxarea());
  pthread_mutex_init(&mtx, NULL);
}

void simplefdn_t::configure()
{
  receivermod_base_t::configure();
  n_channels = AMB11::idx::channels;
  if(fdn)
    delete fdn;
  fdn = new fdn_t(fdnorder, f_sample, logdelays);
  update_par();
  if(foa_out)
    delete foa_out;
  foa_out = new TASCAR::amb1wave_t(n_fragment);
  labels.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    char ctmp[32];
    sprintf(ctmp, ".%d%c", (ch > 0), AMB11::channelorder[ch]);
    labels.push_back(ctmp);
  }
}

void simplefdn_t::add_variables(TASCAR::osc_server_t* srv)
{
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
    distcorr =
        1.0 / (0.5 * pow(volumetric.x * volumetric.y * volumetric.z, 0.33333));
    double dmin(std::min(volumetric.x, std::min(volumetric.y, volumetric.z)) /
                c);
    double dmax(std::max(volumetric.x, std::max(volumetric.y, volumetric.z)) /
                c);
    t = dmin;
    dt = (dmax - dmin);
    if(t60 <= 0.0)
      t60 =
          0.161 * volumetric.boxvolume() / (absorption * volumetric.boxarea());
    if(fdn) {
      double wscale(2.0 * M_PI * t);
      fdn->setpar(wscale * w, wscale * dw, f_sample * t, f_sample * dt,
                  exp(-4.2 * t / t60), std::max(0.0, std::min(0.999, damping)));
    }
    pthread_mutex_unlock(&mtx);
  }
}

void simplefdn_t::setlogdelays(bool ld)
{
  if(pthread_mutex_lock(&mtx) == 0) {
    if(fdn) {
      fdn->set_logdelays(ld);
      double wscale(2.0 * M_PI * t);
      fdn->setpar(wscale * w, wscale * dw, f_sample * t, f_sample * dt,
                  exp(-4.2 * t / t60), std::max(0.0, std::min(0.999, damping)));
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
        output[AMB11::idx::w][t] += fdn->outval.w;
        output[AMB11::idx::x][t] += fdn->outval.x;
        output[AMB11::idx::y][t] += fdn->outval.y;
        output[AMB11::idx::z][t] += fdn->outval.z;
      }
    }
    foa_out->clear();
    pthread_mutex_unlock(&mtx);
  }
}

TASCAR::receivermod_base_t::data_t* simplefdn_t::create_data(double srate,
                                                             uint32_t fragsize)
{
  return new data_t(fragsize);
}

void simplefdn_t::add_pointsource(const TASCAR::pos_t& prel, double width,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>& output,
                                  receivermod_base_t::data_t* sd)
{
  TASCAR::pos_t pnorm(prel.normal());
  data_t* d((data_t*)sd);
  // use ACN everywhere:
  d->_w[AMB11::idx::w] = wgain;
  d->_w[AMB11::idx::x] = pnorm.x;
  d->_w[AMB11::idx::y] = pnorm.y;
  d->_w[AMB11::idx::z] = pnorm.z;
  for(unsigned int k = 0; k < AMB11::idx::channels; k++)
    d->dw[k] = (d->_w[k] - d->w_current[k]) * d->dt;
  for(uint32_t acn = 0; acn < AMB11::idx::channels; ++acn)
    for(uint32_t i = 0; i < chunk.size(); i++)
      (*foa_out)[acn][i] += (d->w_current[acn] += d->dw[acn]) * chunk[i];
}

simplefdn_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k = 0; k < AMB11::idx::channels; k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0 / std::max(1.0, (double)chunksize);
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
