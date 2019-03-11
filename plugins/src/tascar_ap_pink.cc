#include "audioplugin.h"
#include "fft.h"
#include <random>

class pink_t : public TASCAR::audioplugin_base_t {
public:
  pink_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  void prepare( chunk_cfg_t& cf_ );
  void release();
  ~pink_t();
private:
  double fmin;
  double fmax;
  double level;
  double period;
  std::vector<TASCAR::looped_wave_t> pink;
};

pink_t::pink_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    fmin(62.5),
    fmax(4000),
    level(0.001),
    period(4)
{
  GET_ATTRIBUTE(fmin);
  GET_ATTRIBUTE(fmax);
  GET_ATTRIBUTE_DBSPL(level);
  GET_ATTRIBUTE(period);
}

void pink_t::prepare( chunk_cfg_t& cf_ )
{
  TASCAR::audioplugin_base_t::prepare( cf_ );
  uint32_t fftlen(period*f_sample);
  // create 4 buffers, wxyz
  pink.clear();
  pink.emplace_back(fftlen);
  pink.emplace_back(fftlen);
  pink.emplace_back(fftlen);
  pink.emplace_back(fftlen);
  TASCAR::fft_t fft(fftlen);
  TASCAR::spec_t sw(fft.s.n_);
  TASCAR::spec_t sx(fft.s.n_);
  TASCAR::spec_t sy(fft.s.n_);
  TASCAR::spec_t sz(fft.s.n_);
  std::mt19937 gen(1);
  std::uniform_real_distribution<double> dis(0.0, 2*M_PI);
  std::uniform_real_distribution<double> disx(-1.0, 1.0);
  for( uint32_t kf=0;kf<sw.n_;++kf){
    double f((double)kf*f_sample/(double)fftlen);
    if( (f >= fmin) && (f <= fmax) )
      sw.b[kf] = 1.0 / f * cexp(I*dis(gen));
    else
      sw.b[kf] = 0;
    TASCAR::pos_t p(disx(gen),disx(gen),disx(gen));
    p.normalize();
    // FuMa normalization:
    p *= sqrt(2.0);
    sx.b[kf] = p.x*sw.b[kf];
    sy.b[kf] = p.y*sw.b[kf];
    sz.b[kf] = p.z*sw.b[kf];
  }
  // w channel:
  fft.s.copy(sw);
  fft.ifft();
  pink[0].copy(fft.w);
  // x channel:
  fft.s.copy(sx);
  fft.ifft();
  pink[1].copy(fft.w);
  // y channel:
  fft.s.copy(sy);
  fft.ifft();
  pink[2].copy(fft.w);
  // z channel:
  fft.s.copy(sz);
  fft.ifft();
  pink[3].copy(fft.w);
  // level normalization:
  double gain(1.0/pink[0].rms());
  TASCAR::wave_t tmp(n_fragment);
  for( auto it=pink.begin(); it!=pink.end(); ++it ){
    it->set_loop(0);
    (*it) *= gain;
    it->add_chunk_looped(level,tmp);
  }
}

void pink_t::release()
{
}

pink_t::~pink_t()
{
}

void pink_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double_dbspl("/level",&level);
}

void pink_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  for(uint32_t k=0;k<std::min(chunk.size(),pink.size());++k)
    pink[k].add_chunk_looped(level,chunk[k]);
}

REGISTER_AUDIOPLUGIN(pink_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
