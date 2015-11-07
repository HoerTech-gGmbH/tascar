#include <fftw3.h>
#include <complex.h>
#include <string.h>
#include "errorhandling.h"
#include "scene.h"

class hoa2d_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels);
    virtual ~data_t();
    // point source speaker weights:
    uint32_t amb_order;
    float complex* enc_w;
    float complex* enc_dw;
    double dt;
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
  float complex* s_encoded;
  float* s_decoded;
  fftwf_plan dec;
  uint32_t chunk_size;
  uint32_t num_channels;
  float fft_scale;
  //double wgain;
  bool maxre;
  double rotation;
  std::vector<float complex> ordergain;
};

hoa2d_t::hoa2d_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc),
    nbins(spkpos.size()/2+1),
    order(0),
    amb_order(nbins-2),
    s_encoded(NULL),
    s_decoded(NULL),
    chunk_size(0),
    num_channels(0),
    fft_scale(1.0),
    maxre(false),
    rotation(0)
{
  if( spkpos.size() < 3 )
    throw TASCAR::ErrMsg("At least three loudspeakers are required for HOA decoding.");
  GET_ATTRIBUTE(order);
  GET_ATTRIBUTE_DEG(rotation);
  GET_ATTRIBUTE_BOOL(maxre);
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
}

hoa2d_t::~hoa2d_t()
{
  fftwf_destroy_plan(dec);
}

void hoa2d_t::configure(double srate,uint32_t fragsize)
{
  chunk_size = fragsize;
  int32_t channels(spkpos.size());
  num_channels = channels;
  fft_scale = 1.0f/((float)num_channels);
  s_encoded = new float complex[fragsize*nbins];
  s_decoded = new float[fragsize*channels];
  dec = fftwf_plan_many_dft_c2r(1,&channels,fragsize,
                                (fftwf_complex*)s_encoded,NULL,1,nbins,
                                s_decoded,NULL,1,channels,
                                FFTW_ESTIMATE);
  memset(s_encoded,0,sizeof(float)*fragsize*nbins);
  memset(s_decoded,0,sizeof(float)*fragsize*channels);
  //fftwf_print_plan(dec);
  ordergain.resize(nbins);
  //DEBUG(rotation);
  for(uint32_t m=0;m<=amb_order;++m){
    ordergain[m] = fft_scale*cexpf(-(float)m*I*rotation);
    if( maxre )
      ordergain[m] *= cosf((float)m*M_PI/(2.0f*(float)amb_order+2.0f));
  }
}

hoa2d_t::data_t::data_t(uint32_t chunksize,uint32_t channels)
  : amb_order(channels/2-1),
    enc_w(new float complex[amb_order+1]),
    enc_dw(new float complex[amb_order+1])
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
  for(uint32_t ko=1;ko<=amb_order;++ko)
    d->enc_dw[ko] = (ordergain[ko]*cexpf(I*ko*az) - d->enc_w[ko])*d->dt;
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
    //memcpy(p_encode,&(s_encode[kch*fragsize])
  }
  //
  memset(s_encoded,0,sizeof(float complex)*chunk_size*nbins);
  memset(s_decoded,0,sizeof(float)*chunk_size*num_channels);
}

void hoa2d_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  spkpos.foa_decode(chunk,output);
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
  return new data_t(fragsize,spkpos.size());
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
