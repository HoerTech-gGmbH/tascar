#include <fftw3.h>
#include <complex.h>
#include <string.h>
#include "errorhandling.h"
#include "scene.h"

class hoa2d_t : public TASCAR::receivermod_base_speaker_t {
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
  // initialize decoder and allocate buffers:
  void configure(double srate,uint32_t fragsize);
  // decode HOA signals:
  void postproc(std::vector<TASCAR::wave_t>& output);
private:
  //TASCAR::spk_array_t spkpos;
  uint32_t nbins;
  uint32_t order;
  uint32_t amb_order;
  float _Complex* s_encoded;
  float* s_decoded;
  fftwf_plan dec;
  uint32_t chunk_size;
  uint32_t num_channels;
  float fft_scale;
  //double wgain;
  bool maxre;
  double rotation;
  std::vector<float _Complex> ordergain;
  bool diffup;
  double diffup_rot;
  double diffup_delay;
  uint32_t diffup_maxorder;
  uint32_t idelay;
};

hoa2d_t::hoa2d_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc),
    nbins(spkpos.size()/2+1),
    order(0),
    amb_order(nbins-2),
    s_encoded(NULL),
    s_decoded(NULL),
    dec(NULL),
    chunk_size(0),
    num_channels(0),
    fft_scale(1.0),
    maxre(false),
    rotation(0),
    diffup(false),
    diffup_rot(45*DEG2RAD),
    diffup_delay(0.01),
    diffup_maxorder(100),
    idelay(0)
{
  if( spkpos.size() < 3 )
    throw TASCAR::ErrMsg("At least three loudspeakers are required for HOA decoding.");
  GET_ATTRIBUTE(order);
  GET_ATTRIBUTE_DEG(rotation);
  GET_ATTRIBUTE_BOOL(maxre);
  GET_ATTRIBUTE_BOOL(diffup);
  GET_ATTRIBUTE_DEG(diffup_rot);
  GET_ATTRIBUTE(diffup_delay);
  GET_ATTRIBUTE(diffup_maxorder);
  //GET_ATTRIBUTE(wgain);
  if( order > 0 )
    amb_order = std::min((uint32_t)order,amb_order);
}

void hoa2d_t::write_xml()
{
  TASCAR::receivermod_base_speaker_t::write_xml();
  SET_ATTRIBUTE(order);
  SET_ATTRIBUTE_DEG(rotation);
  SET_ATTRIBUTE_BOOL(maxre);
  SET_ATTRIBUTE_BOOL(diffup);
  SET_ATTRIBUTE_DEG(diffup_rot);
  SET_ATTRIBUTE(diffup_delay);
  SET_ATTRIBUTE(diffup_maxorder);
}

hoa2d_t::~hoa2d_t()
{
  if( dec )
    fftwf_destroy_plan(dec);
}

void hoa2d_t::configure(double srate,uint32_t fragsize)
{
  TASCAR::receivermod_base_speaker_t::configure(srate,fragsize);
  chunk_size = fragsize;
  int32_t channels(spkpos.size());
  num_channels = channels;
  fft_scale = 1.0f/((float)num_channels);
  s_encoded = new float _Complex[fragsize*nbins];
  s_decoded = new float[fragsize*channels];
  dec = fftwf_plan_many_dft_c2r(1,&channels,fragsize,
                                (fftwf_complex*)s_encoded,NULL,1,nbins,
                                s_decoded,NULL,1,channels,
                                FFTW_ESTIMATE);
  memset(s_encoded,0,sizeof(float)*fragsize*nbins);
  memset(s_decoded,0,sizeof(float)*fragsize*channels);
  ordergain.resize(nbins);
  for(uint32_t m=0;m<=amb_order;++m){
    ordergain[m] = fft_scale*cexpf(-(float)m*I*rotation);
    if( maxre )
      ordergain[m] *= cosf((float)m*M_PI/(2.0f*(float)amb_order+2.0f));
  }
  idelay = diffup_delay*srate;
}

hoa2d_t::data_t::data_t(uint32_t chunksize,uint32_t channels, double srate)
  : amb_order(channels/2-1),
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
  for(uint32_t ko=1;ko<=amb_order;++ko){
    d->enc_dw[ko] = (ordergain[ko]*ckiaz - d->enc_w[ko])*d->dt;
    ckiaz *= ciaz;
  }
  d->enc_dw[0] = 0.0f;
  //d->enc_w[0] = fft_scale*sqrtf(0.5f);
  d->enc_w[0] = ordergain[0];
  for(uint32_t kt=0;kt<chunk_size;++kt){
    for(uint32_t ko=0;ko<=amb_order;++ko)
      s_encoded[kt*nbins+ko] += (d->enc_w[ko] += d->enc_dw[ko]) * chunk[kt];
  }
}

void hoa2d_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  fftwf_execute(dec);
  // copy to output:
  float* p_encode(s_decoded);
  for(uint32_t kt=0;kt<chunk_size;++kt){
    for(uint32_t kch=0;kch<num_channels;++kch)
      output[kch][kt]+=p_encode[kch];
    p_encode+=num_channels;
  }
  //
  memset(s_encoded,0,sizeof(float _Complex)*chunk_size*nbins);
  memset(s_decoded,0,sizeof(float)*chunk_size*num_channels);
  TASCAR::receivermod_base_speaker_t::postproc(output);  
}

void hoa2d_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float _Complex rot_p(cexpf(I*diffup_rot));
  float _Complex rot_m(cexpf(-I*diffup_rot));
  //spkpos.foa_decode(chunk,output);
  for(uint32_t kt=0;kt<chunk_size;++kt){
    s_encoded[kt*nbins] += ordergain[0]*chunk.w()[kt];
    s_encoded[kt*nbins+1] += ordergain[1]*(chunk.x()[kt] + I*chunk.y()[kt]);
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
    for(uint32_t l=2;l<=std::min(amb_order,diffup_maxorder);++l){
      for(uint32_t k=0;k<n;++k){
        float _Complex tmp1(rot_p*(d->wx_1[k]+I*d->wy_1[k]));
        float _Complex tmp2(rot_m*(d->wx_2[k]+I*d->wy_2[k]));
        d->wx_1[k] = crealf(tmp1);
        d->wx_2[k] = crealf(tmp2);
        d->wy_1[k] = cimag(tmp1);
        d->wy_2[k] = cimag(tmp2);
        s_encoded[k*nbins+l] = ordergain[l]*(tmp1+tmp2);
      }
    }
  }
}

uint32_t hoa2d_t::get_num_channels()
{
  return spkpos.size();
}

std::string hoa2d_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  sprintf(ctmp,".%d%s",channel,spkpos[channel].label.c_str());
  return ctmp;
}

TASCAR::receivermod_base_t::data_t* hoa2d_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize,spkpos.size(),srate);
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
