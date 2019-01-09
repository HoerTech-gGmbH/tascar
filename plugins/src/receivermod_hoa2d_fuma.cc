#include <fftw3.h>
#include <complex.h>
#include <string.h>
#include "errorhandling.h"
#include "scene.h"
#include "amb33defs.h"

class hoa2d_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels, double srate, TASCAR::fsplit_t::shape_t shape, double tau);
    virtual ~data_t();
    // point source speaker weights:
    uint32_t amb_order;
    float _Complex* enc_wp;
    float _Complex* enc_wm;
    float _Complex* enc_dwp;
    float _Complex* enc_dwm;
    double dt;
    TASCAR::wave_t wx_1;
    TASCAR::wave_t wx_2;
    TASCAR::wave_t wy_1;
    TASCAR::wave_t wy_2;
    TASCAR::fsplit_t delay;
    TASCAR::varidelay_t dx;
    TASCAR::varidelay_t dy;
  };
  hoa2d_t(xmlpp::Element* xmlsrc);
  virtual ~hoa2d_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t n_fragment);
  // allocate buffers:
  void prepare( chunk_cfg_t& );
  // re-order HOA signals:
  void postproc(std::vector<TASCAR::wave_t>& output);
private:
  uint32_t nbins;
  uint32_t order;
  float _Complex* s_encoded;
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

hoa2d_t::hoa2d_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    nbins(0),
    order(0),
    s_encoded(NULL),
    diffup(false),
    diffup_rot(45*DEG2RAD),
    diffup_delay(0.01),
    diffup_maxorder(100),
    idelay(0),
    idelaypoint(0),
    filterperiod(0.005)
{
  GET_ATTRIBUTE(order);
  GET_ATTRIBUTE_BOOL(diffup);
  GET_ATTRIBUTE_DEG(diffup_rot);
  GET_ATTRIBUTE(diffup_delay);
  GET_ATTRIBUTE(diffup_maxorder);
  GET_ATTRIBUTE(filterperiod);
  std::string filtershape;
  GET_ATTRIBUTE(filtershape);
  if( filtershape.empty() )
    filtershape = "none";
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
  if( s_encoded )
    delete [] s_encoded;
}

void hoa2d_t::prepare( chunk_cfg_t& cf_ )
{
  TASCAR::receivermod_base_t::prepare( cf_ );
  if( s_encoded )
    delete [] s_encoded;
  s_encoded = new float _Complex[n_fragment*nbins];
  memset(s_encoded,0,sizeof(float _Complex)*n_fragment*nbins);
  idelay = diffup_delay*f_sample;
  idelaypoint = filterperiod*f_sample;
}

hoa2d_t::data_t::data_t(uint32_t chunksize,uint32_t order, double srate, TASCAR::fsplit_t::shape_t shape, double tau)
  : amb_order(order),
    enc_wp(new float _Complex[amb_order+1]),
    enc_wm(new float _Complex[amb_order+1]),
    enc_dwp(new float _Complex[amb_order+1]),
    enc_dwm(new float _Complex[amb_order+1]),
    wx_1(chunksize),
    wx_2(chunksize),
    wy_1(chunksize),
    wy_2(chunksize),
    delay(srate, shape, srate*tau ),
    dx(srate, srate, 340, 0, 0 ),
    dy(srate, srate, 340, 0, 0 )
{
  memset(enc_wp,0,sizeof(float _Complex)*(amb_order+1));
  memset(enc_wm,0,sizeof(float _Complex)*(amb_order+1));
  memset(enc_dwp,0,sizeof(float _Complex)*(amb_order+1));
  memset(enc_dwm,0,sizeof(float _Complex)*(amb_order+1));
  dt = 1.0/std::max(1.0,(double)chunksize);
}

hoa2d_t::data_t::~data_t()
{
  delete [] enc_wp;
  delete [] enc_wm;
  delete [] enc_dwp;
  delete [] enc_dwm;
}

void hoa2d_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float az(-prel.azim());
  if( shape == TASCAR::fsplit_t::none ){
    float _Complex ciazp(cexpf(I*az));
    float _Complex ckiazp(ciazp);
    for(uint32_t ko=1;ko<=order;++ko){
      d->enc_dwp[ko] = (ckiazp - d->enc_wp[ko])*d->dt;
      ckiazp *= ciazp;
    }
    d->enc_dwp[0] = 0.0f;
    d->enc_wp[0] = 1.0f;
    float* vpend(chunk.d+chunk.n);
    float _Complex* encp(s_encoded);
    for(float* vp=chunk.d;vp!=vpend;++vp){
      float _Complex* pwp(d->enc_wp);
      float _Complex* pdwp(d->enc_dwp);
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
    float _Complex ciazp(cexpf(I*(az+width)));
    float _Complex ciazm(cexpf(I*(az-width)));
    float _Complex ckiazp(ciazp);
    float _Complex ckiazm(ciazm);
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
        s_encoded[kt*nbins+ko] += ((d->enc_wp[ko] += d->enc_dwp[ko]) * vp + 
                                   (d->enc_wm[ko] += d->enc_dwm[ko]) * vm);
    }
  }
}

void hoa2d_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  for(uint32_t kt=0;kt<n_fragment;++kt)
    output[0][kt] = MIN3DB*creal(s_encoded[kt*nbins]);
  for(uint32_t ko=1;ko<=order;++ko){
    uint32_t kc(2*ko-1);
    for(uint32_t kt=0;kt<n_fragment;++kt)
      output[kc][kt] = cimag(s_encoded[kt*nbins+ko]);
    ++kc;
    for(uint32_t kt=0;kt<n_fragment;++kt)
      output[kc][kt] = creal(s_encoded[kt*nbins+ko]);
  }
  //
  memset(s_encoded,0,sizeof(float _Complex)*n_fragment*nbins);
  TASCAR::receivermod_base_t::postproc(output);  
}

void hoa2d_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float _Complex rot_p(cexpf(I*diffup_rot));
  float _Complex rot_m(cexpf(-I*diffup_rot));
  for(uint32_t kt=0;kt<n_fragment;++kt){
    s_encoded[kt*nbins] += chunk.w()[kt];
    s_encoded[kt*nbins+1] += (chunk.x()[kt] + I*chunk.y()[kt]);
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
        float _Complex tmp1(rot_p*(d->wx_1[k]+I*d->wy_1[k]));
        float _Complex tmp2(rot_m*(d->wx_2[k]+I*d->wy_2[k]));
        d->wx_1[k] = crealf(tmp1);
        d->wx_2[k] = crealf(tmp2);
        d->wy_1[k] = cimag(tmp1);
        d->wy_2[k] = cimag(tmp2);
        s_encoded[k*nbins+l] = (tmp1+tmp2);
      }
    }
  }
}

uint32_t hoa2d_t::get_num_channels()
{
  return order*2+1;
}

std::string hoa2d_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  uint32_t o((channel+1)/2);
  int32_t s(o*(2*((channel+1) % 2)-1));
  sprintf(ctmp,".%d_%d",o,s);
  return ctmp;
}

TASCAR::receivermod_base_t::data_t* hoa2d_t::create_data(double srate,uint32_t n_fragment)
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
