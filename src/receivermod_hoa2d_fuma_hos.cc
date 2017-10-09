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
    data_t(uint32_t chunksize,uint32_t channels);
    virtual ~data_t();
    // point source speaker weights:
    float _Complex* enc_w;
    float _Complex* enc_dw;
    double gauge;
    double dgauge;
  };
  hoa2d_t(xmlpp::Element* xmlsrc);
  void write_xml();
  virtual ~hoa2d_t();
  //void write_xml();
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
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
  double rho0;
  float _Complex* s_encoded;
  float _Complex* s_encoded_alt;
};

hoa2d_t::hoa2d_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    nbins(0),
    order(0),
    rho0(1),
    s_encoded(NULL),
    s_encoded_alt(NULL)
{
  GET_ATTRIBUTE(order);
  GET_ATTRIBUTE(rho0);
  nbins = order + 2;
}

void hoa2d_t::write_xml()
{
  TASCAR::receivermod_base_t::write_xml();
  SET_ATTRIBUTE(order);
}

hoa2d_t::~hoa2d_t()
{
  if( s_encoded )
    delete [] s_encoded;
  if( s_encoded_alt )
    delete [] s_encoded_alt;
}

void hoa2d_t::prepare( chunk_cfg_t& cf_ )
{
  TASCAR::receivermod_base_t::prepare( cf_ );
  if( s_encoded )
    delete [] s_encoded;
  if( s_encoded_alt )
    delete [] s_encoded_alt;
  s_encoded = new float _Complex[n_fragment*nbins];
  memset(s_encoded,0,sizeof(float _Complex)*n_fragment*nbins);
  s_encoded_alt = new float _Complex[n_fragment*nbins];
  memset(s_encoded_alt,0,sizeof(float _Complex)*n_fragment*nbins);
}

hoa2d_t::data_t::data_t(uint32_t chunksize,uint32_t order)
  : enc_w(new float _Complex[order+1]),
    enc_dw(new float _Complex[order+1]),
    gauge(0),
    dgauge(0)
{
  memset(enc_w,0,sizeof(float _Complex)*(order+1));
  memset(enc_dw,0,sizeof(float _Complex)*(order+1));
}

hoa2d_t::data_t::~data_t()
{
  delete [] enc_w;
  delete [] enc_dw;
}

void hoa2d_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float az(-prel.azim());
  float rho(prel.norm());
  double normd(rho/rho0);
  float _Complex ciazp(cexpf(I*az));
  float _Complex ckiazp(ciazp);
  d->dgauge = (1.0/(1.0+normd) - d->gauge) * t_inc;
  for(uint32_t ko=1;ko<=order;++ko){
    d->enc_dw[ko] = (ckiazp - d->enc_w[ko]) * t_inc;
    ckiazp *= ciazp;
  }
  d->enc_dw[0] = 0.0f;
  d->enc_w[0] = 1.0f;
  float* vpend(chunk.d+chunk.n);
  float _Complex* encp(s_encoded);
  float _Complex* encpa(s_encoded_alt);
  for(float* vp=chunk.d;vp!=vpend;++vp){
    d->gauge += d->dgauge;
    double gaugea(1.0-d->gauge);
    float _Complex* pwp(d->enc_w);
    float _Complex* pdwp(d->enc_dw);
    for(uint32_t ko=0;ko<=order;++ko){
      *pwp += *pdwp;
      *encp += *pwp * (*vp) * d->gauge;
      *encpa += *pwp * (*vp) * gaugea;
      ++encp;
      ++encpa;
      ++pwp;
      ++pdwp;
    }
    for(uint32_t ko=order+1;ko<nbins;++ko){
      ++encp;
      ++encpa;
    }
  }
}

void hoa2d_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  uint32_t ch(order*2+1);
  for(uint32_t kt=0;kt<n_fragment;++kt){
    output[0][kt] = MIN3DB*creal(s_encoded[kt*nbins]);
    output[ch][kt] = MIN3DB*creal(s_encoded_alt[kt*nbins]);
  }
  for(uint32_t ko=1;ko<=order;++ko){
    uint32_t kc(2*ko-1);
    for(uint32_t kt=0;kt<n_fragment;++kt){
      output[kc][kt] = cimag(s_encoded[kt*nbins+ko]);
      output[kc+ch][kt] = cimag(s_encoded_alt[kt*nbins+ko]);
    }
    ++kc;
    for(uint32_t kt=0;kt<n_fragment;++kt){
      output[kc][kt] = creal(s_encoded[kt*nbins+ko]);
      output[kc+ch][kt] = creal(s_encoded_alt[kt*nbins+ko]);
    }
  }
  //
  memset(s_encoded,0,sizeof(float _Complex)*n_fragment*nbins);
  memset(s_encoded_alt,0,sizeof(float _Complex)*n_fragment*nbins);
  TASCAR::receivermod_base_t::postproc(output);  
}

void hoa2d_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  for(uint32_t kt=0;kt<n_fragment;++kt){
    s_encoded[kt*nbins] += chunk.w()[kt];
    s_encoded[kt*nbins+1] += (chunk.x()[kt] + I*chunk.y()[kt]);
  }
}

uint32_t hoa2d_t::get_num_channels()
{
  return 2*(order*2+1);
}

std::string hoa2d_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  char ctmpa[2];
  ctmpa[1] = 0;
  ctmpa[0] = 0;
  if( channel >= (2*order+1) ){
    channel -= 2*order+1;
    ctmpa[0] = 'a';
  }
  uint32_t o((channel+1)/2);
  int32_t s(o*(2*((channel+1) % 2)-1));
  sprintf(ctmp,".%s%d_%d",ctmpa,o,s);
  return ctmp;
}

TASCAR::receivermod_base_t::data_t* hoa2d_t::create_data(double srate,uint32_t n_fragment)
{
  return new data_t( n_fragment, order );
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
