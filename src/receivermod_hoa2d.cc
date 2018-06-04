#include <fftw3.h>
#include <complex.h>
#include <string.h>
#include "errorhandling.h"
#include "scene.h"

class hoa2d_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t( uint32_t chunksize, uint32_t channels, double srate, TASCAR::fsplit_t::shape_t shape, double tau );
    virtual ~data_t();
    // point source speaker weights:
    uint32_t amb_order;
    float _Complex* enc_wp;
    float _Complex* enc_wm;
    float _Complex* enc_dwp;
    float _Complex* enc_dwm;
    TASCAR::wave_t wx_1;
    TASCAR::wave_t wx_2;
    TASCAR::wave_t wy_1;
    TASCAR::wave_t wy_2;
    TASCAR::fsplit_t fsplitdelay;
    TASCAR::varidelay_t dx;
    TASCAR::varidelay_t dy;
  };
  hoa2d_t(xmlpp::Element* xmlsrc);
  virtual ~hoa2d_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  // initialize decoder and allocate buffers:
  void prepare( chunk_cfg_t& );
  // decode HOA signals:
  void postproc(std::vector<TASCAR::wave_t>& output);
  void add_variables( TASCAR::osc_server_t* srv );
private:
  uint32_t nbins;
  uint32_t order;
  uint32_t amb_order;
  float _Complex* s_encoded;
  float* s_decoded;
  fftwf_plan dec;
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
    fft_scale(1.0),
    //wgain(sqrt(2.0)),
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
  //GET_ATTRIBUTE(wgain);
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

void hoa2d_t::add_variables( TASCAR::osc_server_t* srv )
{
  //srv->add_double( "/wgain", &wgain );
  srv->add_bool( "/diffup", &diffup );
  srv->add_double_degree( "/diffup_rot", &diffup_rot );
  srv->add_double( "/diffup_delay", &diffup_delay );
  srv->add_uint( "/diffup_maxorder", &diffup_maxorder );
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

void hoa2d_t::prepare( chunk_cfg_t& cf_ )
{
  TASCAR::receivermod_base_speaker_t::prepare( cf_ );
  n_channels = spkpos.size();
  update();
  fft_scale = 1.0f/((float)n_channels);
  if( s_encoded )
    delete [] s_encoded;
  if( s_decoded )
    delete [] s_decoded;
  s_encoded = new float _Complex[n_fragment * nbins];
  s_decoded = new float[ n_fragment * n_channels];
  int ichannels(n_channels);
  dec = fftwf_plan_many_dft_c2r(1,&ichannels,n_fragment,
                                (fftwf_complex*)s_encoded,NULL,1,nbins,
                                s_decoded,NULL,1,n_channels,
                                FFTW_ESTIMATE);
  memset(s_encoded,0,sizeof(float _Complex)*n_fragment*nbins);
  memset(s_decoded,0,sizeof(float)*n_fragment*n_channels);
  ordergain.resize(nbins);
  for(uint32_t m=0;m<=amb_order;++m){
    ordergain[m] = fft_scale*cexpf(-(float)m*I*rotation);
    if( maxre )
      ordergain[m] *= cosf((float)m*M_PI/(2.0f*(float)amb_order+2.0f));
  }
  idelay = diffup_delay*f_sample;
  idelaypoint = filterperiod*f_sample;
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
    fsplitdelay(srate, shape, srate*tau ),
    dx(srate, srate, 340, 0, 0 ),
    dy(srate, srate, 340, 0, 0 )
{
  memset(enc_wp,0,sizeof(float _Complex)*(amb_order+1));
  memset(enc_wm,0,sizeof(float _Complex)*(amb_order+1));
  memset(enc_dwp,0,sizeof(float _Complex)*(amb_order+1));
  memset(enc_dwm,0,sizeof(float _Complex)*(amb_order+1));
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
    for(uint32_t ko=1;ko<=amb_order;++ko){
      d->enc_dwp[ko] = (ordergain[ko]*ckiazp - d->enc_wp[ko])*t_inc;
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
      d->enc_dwp[ko] = (ordergain[ko]*ckiazp - d->enc_wp[ko])*t_inc;
      ckiazp *= ciazp;
      d->enc_dwm[ko] = (ordergain[ko]*ckiazm - d->enc_wm[ko])*t_inc;
      ckiazm *= ciazm;
    }
    d->enc_dwp[0] = d->enc_dwm[0] = 0.0f;
    d->enc_wp[0] = d->enc_wm[0] = ordergain[0];
    for(uint32_t kt=0;kt<n_fragment;++kt){
      d->fsplitdelay.push(chunk[kt]);
      float vp, vm;
      d->fsplitdelay.get(vp,vm);
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
  for(uint32_t kt=0;kt<n_fragment;++kt){
    for(uint32_t kch=0;kch<n_channels;++kch)
      output[kch][kt]+=p_encode[kch];
    p_encode+=n_channels;
  }
  //
  memset(s_encoded,0,sizeof(float _Complex)*n_fragment*nbins);
  memset(s_decoded,0,sizeof(float)*n_fragment*n_channels);
  TASCAR::receivermod_base_speaker_t::postproc(output);  
}

void hoa2d_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  idelay = diffup_delay*f_sample;
  data_t* d((data_t*)sd);
  // copy first order data:
  float wgain(fft_scale*sqrt(2.0));
  float xyzgain(fft_scale*0.5);
  for(uint32_t kt=0;kt<n_fragment;++kt){
    s_encoded[kt*nbins] += wgain*chunk.w()[kt];
    s_encoded[kt*nbins+1] += xyzgain*(chunk.x()[kt] + I*chunk.y()[kt]);
  }
  if( diffup ){
    float _Complex rot_p(cexpf(I*diffup_rot));
    float _Complex rot_m(cexpf(-I*diffup_rot));
    // create filtered x and y signals:
    uint32_t n(chunk.size());
    for(uint32_t kt=0;kt<n;++kt){
      // fill delayline:
      float xin(chunk.x()[kt]);
      float yin(chunk.y()[kt]);
      d->dx.push(xin);
      d->dy.push(yin);
      // get delayed value:
      float xdelayed(d->dx.get(idelay));
      float ydelayed(d->dy.get(idelay));
      // create filtered signals:
      float _Complex tmp_p(0.5*((xin + xdelayed) + I*(yin + ydelayed)));
      float _Complex tmp_m(0.5*((xin - xdelayed) + I*(yin - ydelayed)));
      for(uint32_t l=2;l<=std::min(amb_order,diffup_maxorder);++l){
        tmp_p *= rot_p;
        tmp_m *= rot_m;
        s_encoded[kt*nbins+l] += ordergain[l]*(tmp_p+tmp_m);
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
  return new data_t( fragsize, spkpos.size(), srate, shape, filterperiod );
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
