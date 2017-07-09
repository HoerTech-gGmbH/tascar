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
    data_t(uint32_t chunksize,uint32_t channels, double srate);
    virtual ~data_t();
    // point source speaker weights:
    uint32_t amb_order;
    float _Complex* enc_w;
    float _Complex* enc_dw;
    double dt;
    TASCAR::wave_t wx_1;
    TASCAR::wave_t wx_2;
    TASCAR::wave_t wy_1;
    TASCAR::wave_t wy_2;
    TASCAR::varidelay_t dx;
    TASCAR::varidelay_t dy;
  };
  hoa2d_t(xmlpp::Element* xmlsrc);
  void write_xml();
  virtual ~hoa2d_t();
  //void write_xml();
  void add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  // allocate buffers:
  void configure(double srate,uint32_t fragsize);
  // reorder HOA signals:
  void postproc(std::vector<TASCAR::wave_t>& output);
private:
  uint32_t nbins;
  uint32_t order;
  float _Complex* s_encoded;
  uint32_t chunk_size;
  uint32_t num_channels;
  bool diffup;
  double diffup_rot;
  double diffup_delay;
  uint32_t diffup_maxorder;
  uint32_t idelay;
};

hoa2d_t::hoa2d_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    nbins(0),
    order(0),
    s_encoded(NULL),
    chunk_size(0),
    num_channels(0),
    diffup(false),
    diffup_rot(45*DEG2RAD),
    diffup_delay(0.01),
    diffup_maxorder(100),
    idelay(0)
{
  GET_ATTRIBUTE(order);
  GET_ATTRIBUTE_BOOL(diffup);
  GET_ATTRIBUTE_DEG(diffup_rot);
  GET_ATTRIBUTE(diffup_delay);
  GET_ATTRIBUTE(diffup_maxorder);
  nbins = order + 2;
  num_channels = order*2+1;
}

void hoa2d_t::write_xml()
{
}

hoa2d_t::~hoa2d_t()
{
  if( s_encoded )
    delete [] s_encoded;
}

void hoa2d_t::configure(double srate,uint32_t fragsize)
{
  TASCAR::receivermod_base_t::configure(srate,fragsize);
  chunk_size = fragsize;
  if( s_encoded )
    delete [] s_encoded;
  s_encoded = new float _Complex[fragsize*nbins];
  memset(s_encoded,0,sizeof(float _Complex)*fragsize*nbins);
  idelay = diffup_delay*srate;
}

hoa2d_t::data_t::data_t(uint32_t chunksize,uint32_t channels, double srate)
  : amb_order((channels-1)/2),
    enc_w(new float _Complex[amb_order+1]),
    enc_dw(new float _Complex[amb_order+1]),
    wx_1(chunksize),
    wx_2(chunksize),
    wy_1(chunksize),
    wy_2(chunksize),
    dx(srate, srate, 340, 0, 0 ),
    dy(srate, srate, 340, 0, 0 )
{
  for(uint32_t k=0;k<=amb_order;++k)
    enc_w[k] = enc_dw[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

hoa2d_t::data_t::~data_t()
{
  delete [] enc_w;
  delete [] enc_dw;
}

void hoa2d_t::add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  if( !chunk_size )
    throw TASCAR::ErrMsg("Configure method not called or not configured properly.");
  data_t* d((data_t*)sd);
  float az(-prel.azim());
  float _Complex ciaz(cexpf(I*az));
  float _Complex ckiaz(ciaz);
  for(uint32_t ko=1;ko<=order;++ko){
    d->enc_dw[ko] = (ckiaz - d->enc_w[ko])*d->dt;
    ckiaz *= ciaz;
  }
  d->enc_dw[0] = 0.0f;
  d->enc_w[0] = 1.0f;
  for(uint32_t kt=0;kt<chunk_size;++kt){
    for(uint32_t ko=0;ko<=order;++ko)
      s_encoded[kt*nbins+ko] += (d->enc_w[ko] += d->enc_dw[ko]) * chunk[kt];
  }
}

void hoa2d_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  for(uint32_t kt=0;kt<chunk_size;++kt)
    output[0][kt] = MIN3DB*creal(s_encoded[kt*nbins]);
  for(uint32_t ko=1;ko<=order;++ko){
    uint32_t kc(2*ko-1);
    for(uint32_t kt=0;kt<chunk_size;++kt)
      output[kc][kt] = cimag(s_encoded[kt*nbins+ko]);
    ++kc;
    for(uint32_t kt=0;kt<chunk_size;++kt)
      output[kc][kt] = creal(s_encoded[kt*nbins+ko]);
  }
  //
  memset(s_encoded,0,sizeof(float _Complex)*chunk_size*nbins);
  TASCAR::receivermod_base_t::postproc(output);  
}

void hoa2d_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float _Complex rot_p(cexpf(I*diffup_rot));
  float _Complex rot_m(cexpf(-I*diffup_rot));
  //spkpos.foa_decode(chunk,output);
  for(uint32_t kt=0;kt<chunk_size;++kt){
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
  return num_channels;
}

std::string hoa2d_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  uint32_t o((channel+1)/2);
  int32_t s(o*(2*((channel+1) % 2)-1));
  sprintf(ctmp,".%d_%d",o,s);
  return ctmp;
}

TASCAR::receivermod_base_t::data_t* hoa2d_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize,num_channels,srate);
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
