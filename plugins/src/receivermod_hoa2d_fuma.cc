#include <fftw3.h>
#include <string.h>
#include "errorhandling.h"
#include "scene.h"
#include "amb33defs.h"

const std::complex<double> i(0.0, 1.0);
const std::complex<float> i_f(0.0, 1.0);

class hoa2d_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels, double srate, TASCAR::fsplit_t::shape_t shape, double tau);
    virtual ~data_t();
    // point source speaker weights:
    uint32_t amb_order;
    TASCAR::spec_t enc_wp;
    TASCAR::spec_t enc_wm;
    TASCAR::spec_t enc_dwp;
    TASCAR::spec_t enc_dwm;
    float dt;
    TASCAR::wave_t wx_1;
    TASCAR::wave_t wx_2;
    TASCAR::wave_t wy_1;
    TASCAR::wave_t wy_2;
    TASCAR::fsplit_t delay;
    TASCAR::varidelay_t dx;
    TASCAR::varidelay_t dy;
  };
  hoa2d_t(tsccfg::node_t xmlsrc);
  virtual ~hoa2d_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t n_fragment) const;
  // allocate buffers:
  void configure();
  // re-order HOA signals:
  void postproc(std::vector<TASCAR::wave_t>& output);
private:
  uint32_t nbins;
  uint32_t order;
  TASCAR::spec_t s_encoded;
  bool diffup;
  double diffup_rot;
  double diffup_delay;
  uint32_t diffup_maxorder;
  uint32_t idelay;
  uint32_t idelaypoint;
  // Zotter width control:
  //uint32_t truncofs; //< truncation offset
  //double dispersion; //< dispersion constant (to be replaced by physical object size)
  //double d0; //< distance at which the dispersion constant is achieved
  double filterperiod; //< filter period time in seconds
  TASCAR::fsplit_t::shape_t shape;
};

hoa2d_t::hoa2d_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    nbins(0),
    order(0),
    s_encoded(1),
    diffup(false),
    diffup_rot(45*DEG2RAD),
    diffup_delay(0.01),
    diffup_maxorder(100),
    idelay(0),
    idelaypoint(0),
    filterperiod(0.005)
{
  GET_ATTRIBUTE(order,"","Ambisonics order; 0: use maximum possible");
  GET_ATTRIBUTE_BOOL(diffup,"Use diffuse upsampling similar to \\citet{Zotter2014}");
  GET_ATTRIBUTE_DEG(diffup_rot,"Decorrelation rotation");
  GET_ATTRIBUTE(diffup_delay,"s","Decorrelation delay");
  GET_ATTRIBUTE(diffup_maxorder,"","Maximum order of diffuse sound fields");
  GET_ATTRIBUTE(filterperiod,"s","Filter period for source width encoding");
  std::string filtershape("none");
  GET_ATTRIBUTE(filtershape,"","De-correlation filter shape for source width encoding, one of ``none'', ``notch'', ``sine'', ``tria'', ``triald''");
  if( filtershape == "none" )
    shape = TASCAR::fsplit_t::none;
  else if( filtershape == "notch" )
    shape = TASCAR::fsplit_t::notch;
  else if( filtershape == "sine" )
    shape = TASCAR::fsplit_t::sine;
  else if( filtershape == "tria" )
    shape = TASCAR::fsplit_t::tria;
  else if( filtershape == "triald" )
    shape = TASCAR::fsplit_t::triald;
  else 
    throw TASCAR::ErrMsg("Invalid shape: "+filtershape);
  nbins = order + 2;
}

hoa2d_t::~hoa2d_t()
{
}

void hoa2d_t::configure()
{
  TASCAR::receivermod_base_t::configure();
  n_channels = order*2+1;
  s_encoded.resize(n_fragment*nbins);
  s_encoded.clear();
  idelay = diffup_delay*f_sample;
  idelaypoint = filterperiod*f_sample;
  labels.clear();
  for(uint32_t ch=0;ch<n_channels;++ch){
    char ctmp[1024];
    uint32_t o((ch+1)/2);
    int32_t s(o*(2*((ch+1) % 2)-1));
    sprintf(ctmp,".%d_%d",o,s);
    labels.push_back(ctmp);
  }
}

hoa2d_t::data_t::data_t(uint32_t chunksize,uint32_t order, double srate, TASCAR::fsplit_t::shape_t shape, double tau)
  : amb_order(order),
    enc_wp(amb_order+1),
    enc_wm(amb_order+1),
    enc_dwp(amb_order+1),
    enc_dwm(amb_order+1),
    wx_1(chunksize),
    wx_2(chunksize),
    wy_1(chunksize),
    wy_2(chunksize),
    delay(srate, shape, srate*tau ),
    dx(srate, srate, 340, 0, 0 ),
    dy(srate, srate, 340, 0, 0 )
{
  dt = 1.0/std::max(1.0,(double)chunksize);
}

hoa2d_t::data_t::~data_t()
{
}

void hoa2d_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float az(-prel.azim());
  if( shape == TASCAR::fsplit_t::none ){
    std::complex<float> ciazp(std::exp(i_f*az));
    std::complex<float> ckiazp(ciazp);
    for(uint32_t ko=1;ko<=order;++ko){
      d->enc_dwp[ko] = (ckiazp - d->enc_wp[ko])*d->dt;
      ckiazp *= ciazp;
    }
    d->enc_dwp[0] = 0.0f;
    d->enc_wp[0] = 1.0f;
    float* vpend(chunk.d+chunk.n);
    std::complex<float>* encp(s_encoded.b);
    for(float* vp=chunk.d;vp!=vpend;++vp){
      std::complex<float>* pwp(d->enc_wp.b);
      std::complex<float>* pdwp(d->enc_dwp.b);
      for(uint32_t ko=0;ko<=order;++ko){
        *encp += ((*pwp += *pdwp) * (*vp));
        ++encp;
        ++pwp;
        ++pdwp;
      }
      for(uint32_t ko=order+1;ko<nbins;++ko)
        ++encp;
    }
  }else{
    std::complex<float> ciazp(std::exp(i*(az+width)));
    std::complex<float> ciazm(std::exp(i*(az-width)));
    std::complex<float> ckiazp(ciazp);
    std::complex<float> ckiazm(ciazm);
    for(uint32_t ko=1;ko<=order;++ko){
      d->enc_dwp[ko] = (ckiazp - d->enc_wp[ko])*d->dt;
      ckiazp *= ciazp;
      d->enc_dwm[ko] = (ckiazm - d->enc_wm[ko])*d->dt;
      ckiazm *= ciazm;
    }
    d->enc_dwp[0] = d->enc_dwm[0] = 0.0f;
    d->enc_wp[0] = d->enc_wm[0] = 1.0f;
    for(uint32_t kt=0;kt<n_fragment;++kt){
      d->delay.push(chunk[kt]);
      float vp, vm;
      d->delay.get(vp,vm);
      for(uint32_t ko=0;ko<=order;++ko)
        s_encoded.b[kt*nbins+ko] += ((d->enc_wp[ko] += d->enc_dwp[ko]) * vp + 
                                     (d->enc_wm[ko] += d->enc_dwm[ko]) * vm);
    }
  }
}

void hoa2d_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  for(uint32_t kt=0;kt<n_fragment;++kt)
    output[0][kt] = MIN3DB*s_encoded.b[kt*nbins].real();
  for(uint32_t ko=1;ko<=order;++ko){
    uint32_t kc(2*ko-1);
    for(uint32_t kt=0;kt<n_fragment;++kt)
      output[kc][kt] = s_encoded.b[kt*nbins+ko].imag();
    ++kc;
    for(uint32_t kt=0;kt<n_fragment;++kt)
      output[kc][kt] = s_encoded.b[kt*nbins+ko].real();
  }
  //
  s_encoded.clear();
  TASCAR::receivermod_base_t::postproc(output);  
}

void hoa2d_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  std::complex<float> rot_p(std::exp(i*diffup_rot));
  std::complex<float> rot_m(std::exp(-i*diffup_rot));
  for(uint32_t kt=0;kt<n_fragment;++kt){
    s_encoded.b[kt*nbins] += chunk.w()[kt];
    s_encoded.b[kt*nbins+1] += (chunk.x()[kt] + i_f*chunk.y()[kt]);
  }
  if( diffup ){
    // create filtered x and y signals:
    uint32_t n(chunk.size());
    for(uint32_t k=0;k<n;++k){
      // fill delayline:
      float xin(chunk.x()[k]);
      float yin(chunk.y()[k]);
      d->dx.push(xin);
      d->dy.push(yin);
      // get delayed value:
      float xdelayed(d->dx.get(idelay));
      float ydelayed(d->dy.get(idelay));
      d->wx_1[k] = 0.5*(xin + xdelayed);
      d->wx_2[k] = 0.5*(xin - xdelayed);
      d->wy_1[k] = 0.5*(yin + ydelayed);
      d->wy_2[k] = 0.5*(yin - ydelayed);
    }
    for(uint32_t l=2;l<=std::min(order,diffup_maxorder);++l){
      for(uint32_t k=0;k<n;++k){
        std::complex<float> tmp1(rot_p*(d->wx_1[k]+i_f*d->wy_1[k]));
        std::complex<float> tmp2(rot_m*(d->wx_2[k]+i_f*d->wy_2[k]));
        d->wx_1[k] = tmp1.real();
        d->wx_2[k] = tmp2.real();
        d->wy_1[k] = tmp1.imag();
        d->wy_2[k] = tmp2.imag();
        s_encoded.b[k*nbins+l] = (tmp1+tmp2);
      }
    }
  }
}

TASCAR::receivermod_base_t::data_t* hoa2d_t::create_state_data(double srate,uint32_t n_fragment) const
{
  return new data_t(n_fragment,order,f_sample, shape, filterperiod );
}

REGISTER_RECEIVERMOD(hoa2d_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
