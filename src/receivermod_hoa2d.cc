#include <fftw3.h>
#include <complex.h>
#include <string.h>
#include "errorhandling.h"
#include "scene.h"

class hoa2d_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels, double srate, TASCAR::fsplit_t::shape_t shape, double tau );
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
  void write_xml();
  virtual ~hoa2d_t();
  //void write_xml();
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  // initialize decoder and allocate buffers:
  void configure(double srate,uint32_t fragsize);
  // decode HOA signals:
  void postproc(std::vector<TASCAR::wave_t>& output);
private:
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
  uint32_t idelaypoint;
  // Zotter width control:
  //uint32_t truncofs; //< truncation offset
  //double dispersion; //< dispersion constant (to be replaced by physical object size)
  //double d0; //< distance at which the dispersion constant is achieved
  double filterperiod; //< filter period time in seconds
  TASCAR::fsplit_t::shape_t shape;
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
    rotation(-12345),
    diffup(false),
    diffup_rot(45*DEG2RAD),
    diffup_delay(0.01),
    diffup_maxorder(100),
    idelay(0),
    idelaypoint(0),
    filterperiod(0.005)
{
  if( spkpos.size() < 3 )
    throw TASCAR::ErrMsg("At least three loudspeakers are required for HOA decoding.");
  GET_ATTRIBUTE(order);
  GET_ATTRIBUTE_DEG(rotation);
  if( rotation == -12345 )
    rotation = -spkpos.mean_rotation;
  GET_ATTRIBUTE_BOOL(maxre);
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
  SET_ATTRIBUTE(filterperiod);
}

hoa2d_t::~hoa2d_t()
{
  if( dec )
    fftwf_destroy_plan(dec);
  if( s_encoded )
    delete [] s_encoded;
  if( s_decoded )
    delete [] s_decoded;
}

void hoa2d_t::configure(double srate,uint32_t fragsize)
{
  TASCAR::receivermod_base_speaker_t::configure(srate,fragsize);
  chunk_size = fragsize;
  int32_t channels(spkpos.size());
  num_channels = channels;
  fft_scale = 1.0f/((float)num_channels);
  if( s_encoded )
    delete [] s_encoded;
  if( s_decoded )
    delete [] s_decoded;
  s_encoded = new float _Complex[fragsize*nbins];
  s_decoded = new float[fragsize*channels];
  dec = fftwf_plan_many_dft_c2r(1,&channels,fragsize,
                                (fftwf_complex*)s_encoded,NULL,1,nbins,
                                s_decoded,NULL,1,channels,
                                FFTW_ESTIMATE);
  memset(s_encoded,0,sizeof(float _Complex)*fragsize*nbins);
  memset(s_decoded,0,sizeof(float)*fragsize*channels);
  ordergain.resize(nbins);
  for(uint32_t m=0;m<=amb_order;++m){
    ordergain[m] = fft_scale*cexpf(-(float)m*I*rotation);
    if( maxre )
      ordergain[m] *= cosf((float)m*M_PI/(2.0f*(float)amb_order+2.0f));
  }
  idelay = diffup_delay*srate;
  idelaypoint = filterperiod*srate;
}

hoa2d_t::data_t::data_t(uint32_t chunksize,uint32_t channels, double srate, TASCAR::fsplit_t::shape_t shape, double tau)
  : amb_order((channels-1)/2),
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
  if( !chunk_size )
    throw TASCAR::ErrMsg("Configure method not called or not configured properly.");
  data_t* d((data_t*)sd);
  float az(-prel.azim());
  if( shape == TASCAR::fsplit_t::none ){
    float _Complex ciazp(cexpf(I*az));
    float _Complex ckiazp(ciazp);
    for(uint32_t ko=1;ko<=amb_order;++ko){
      d->enc_dwp[ko] = (ordergain[ko]*ckiazp - d->enc_wp[ko])*d->dt;
      ckiazp *= ciazp;
    }
    d->enc_dwp[0] = 0.0f;
    d->enc_wp[0] = ordergain[0];
    float* vpend(chunk.d+chunk.n);
    float _Complex* encp(s_encoded);
    for(float* vp=chunk.d;vp!=vpend;++vp){
      float _Complex* pwp(d->enc_wp);
      float _Complex* pdwp(d->enc_dwp);
      for(uint32_t ko=0;ko<=amb_order;++ko){
        *encp += ((*pwp += *pdwp) * (*vp));
        ++encp;
        ++pwp;
        ++pdwp;
      }
      for(uint32_t ko=amb_order+1;ko<nbins;++ko)
        ++encp;
    }
  }else{
    float _Complex ciazp(cexpf(I*(az+width)));
    float _Complex ciazm(cexpf(I*(az-width)));
    float _Complex ckiazp(ciazp);
    float _Complex ckiazm(ciazm);
    for(uint32_t ko=1;ko<=amb_order;++ko){
      d->enc_dwp[ko] = (ordergain[ko]*ckiazp - d->enc_wp[ko])*d->dt;
      ckiazp *= ciazp;
      d->enc_dwm[ko] = (ordergain[ko]*ckiazm - d->enc_wm[ko])*d->dt;
      ckiazm *= ciazm;
    }
    d->enc_dwp[0] = d->enc_dwm[0] = 0.0f;
    d->enc_wp[0] = d->enc_wm[0] = ordergain[0];
    for(uint32_t kt=0;kt<chunk_size;++kt){
      d->delay.push(chunk[kt]);
      float vp, vm;
      d->delay.get(vp,vm);
      for(uint32_t ko=0;ko<=amb_order;++ko)
        s_encoded[kt*nbins+ko] += ((d->enc_wp[ko] += d->enc_dwp[ko]) * vp + 
                                   (d->enc_wm[ko] += d->enc_dwm[ko]) * vm);
    }
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
  return new data_t(fragsize,spkpos.size(),srate, shape, filterperiod );
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
