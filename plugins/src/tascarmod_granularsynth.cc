/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "session.h"
#include "ola.h"
#include <stdlib.h>

class granularsynth_vars_t : public TASCAR::module_base_t {
public:
  granularsynth_vars_t( const TASCAR::module_cfg_t& cfg );
  ~granularsynth_vars_t(){};
protected:
  std::string id;
  float wet;
  uint32_t wlen;
  double f0;
  uint32_t numgrains;
  double t0;
  double bpm;
  double loop;
  double gain;
  double ponset;
  double psustain;
  bool active_;
  bool bypass_;
  std::vector<double> pitches;
  std::vector<double> durations;
};

granularsynth_vars_t::granularsynth_vars_t( const TASCAR::module_cfg_t& cfg )
  : TASCAR::module_base_t(cfg),
  id("granularsynth"),
  wet(1.0),
  wlen(8192),
  f0(415.0),
  numgrains(100),
  t0(0),
  bpm(120),
  loop(64),
  gain(1.0),
  ponset(1.0),
  psustain(0.0),
  active_(true),
  bypass_(false)
{
  GET_ATTRIBUTE_(id);
  GET_ATTRIBUTE_(wet);
  GET_ATTRIBUTE_(wlen);
  GET_ATTRIBUTE_(f0);
  GET_ATTRIBUTE_(pitches);
  GET_ATTRIBUTE_(durations);
  GET_ATTRIBUTE_(numgrains);
  GET_ATTRIBUTE_(t0);
  GET_ATTRIBUTE_(bpm);
  GET_ATTRIBUTE_(loop);
  GET_ATTRIBUTE_(ponset);
  GET_ATTRIBUTE_(psustain);
  GET_ATTRIBUTE_DB_(gain);
  get_attribute_bool("active",active_,"","active");
  get_attribute_bool("bypass",bypass_,"","bypass");
}

class grainloop_t : public std::vector<TASCAR::spec_t*>
{
public:
  grainloop_t( double f0, uint32_t nbins, uint32_t ngrains );
  grainloop_t(const grainloop_t& src);
  ~grainloop_t();
  void add(const TASCAR::spec_t& s);
  void play(TASCAR::spec_t& s, double gain);
  void reset() { num_filled = 0; fill_next = 0;num_next = 0;};
private:
  size_t num_filled;
  size_t num_next;
  size_t fill_next;
  uint32_t nbins_;
  uint32_t ngrains_;
public:
  double f;
};

grainloop_t::grainloop_t(const grainloop_t& src)
    : std::vector<TASCAR::spec_t*>(), num_filled(0), num_next(0), fill_next(0),
      nbins_(src.nbins_), ngrains_(src.ngrains_), f(src.f)
{
  for(uint32_t k = 0; k < ngrains_; ++k)
    push_back(new TASCAR::spec_t(nbins_));
}

grainloop_t::grainloop_t( double f0, uint32_t nbins, uint32_t ngrains )
  : num_filled(0),
    num_next(0),
    fill_next(0),
    nbins_(nbins),
    ngrains_(ngrains),
    f(f0)
{
  for( uint32_t k=0;k<ngrains;++k){
    push_back(new TASCAR::spec_t(nbins));
  }
}

grainloop_t::~grainloop_t()
{
  for( auto it=rbegin();it!=rend();++it){
    delete *it;
  }
}

void grainloop_t::add(const TASCAR::spec_t& s)
{
  if( fill_next < size() ){
    operator[](fill_next)->copy(s);
    ++fill_next;
    if( num_filled < size() )
      ++num_filled;
  }
  if( fill_next >= size() )
    fill_next = 0;
}

void grainloop_t::play(TASCAR::spec_t& s, double gain)
{
  if( num_next < std::min(size(),num_filled) ){
    s.add_scaled(*(operator[](num_next)), gain);
    ++num_next;
  }
  if( num_next >= std::min(size(),num_filled) )
    num_next = 0;
}

class granularsynth_t : public granularsynth_vars_t, public jackc_db_t {
public:
  granularsynth_t( const TASCAR::module_cfg_t& cfg );
  ~granularsynth_t();
  virtual int inner_process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
  virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
  static int osc_apply(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void set_apply(float t);
protected:
  std::vector<grainloop_t> grains;
  TASCAR::ola_t ola1;
  TASCAR::ola_t ola2;
  TASCAR::minphase_t minphase;
  //TASCAR::wave_t phase;
  float delaystate;
  TASCAR::wave_t in_delayed;
  uint32_t t_apply;
  float deltaw;
  float currentw;
  std::vector<uint64_t> startingtimes;
  std::vector<uint64_t> idurations;
  std::vector<double> vfreqs;
  uint64_t tprev;
  bool reset;
}; 

granularsynth_t::granularsynth_t( const TASCAR::module_cfg_t& cfg )
  : granularsynth_vars_t( cfg ),
    jackc_db_t( id, wlen ),
    //ola_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize, windowtype_t wnd, windowtype_t zerownd,double wndpos,windowtype_t postwnd=WND_RECT);
    ola1(4*wlen,4*wlen,wlen,TASCAR::stft_t::WND_HANNING,TASCAR::stft_t::WND_RECT,0.5),
    ola2(4*wlen,4*wlen,wlen,TASCAR::stft_t::WND_HANNING,TASCAR::stft_t::WND_RECT,0.5),
    minphase(4*wlen),
    delaystate(0.0f),
    in_delayed(wlen),
    t_apply(0),
    deltaw(0),
    currentw(0),
    tprev(0),
    reset(false)
{
  std::set<double> vf;
  // convert semitone pitches to frequencies in Hz, and store unique
  // list of frequencies:
  for( auto it=pitches.begin();it!=pitches.end();++it){
    vfreqs.push_back(f0*pow(2.0,(*it)/12.0));
    vf.insert(vfreqs.back());
  }
  // for each frequency, allocate a grainloop:
  for( auto it=vf.begin();it!=vf.end();++it )
    grains.push_back(grainloop_t( *it, ola1.s.n_, numgrains ));
  // calculate start times from durations:
  uint64_t t(0);
  for( auto it=durations.begin();it!=durations.end();++it){
    idurations.push_back( srate * (*it) / (bpm/60.0) );
    startingtimes.push_back( t );
    t += idurations.back();
  }
  // create jack ports:
  add_input_port("in");
  add_output_port("out");
  // register OSC variables:
  session->add_float("/c/"+id+"/wet",&wet);
  session->add_method("/c/"+id+"/wetapply","f",&granularsynth_t::osc_apply,this);
  session->add_double("/c/"+id+"/t0",&t0);
  session->add_double("/c/"+id+"/ponset",&ponset);
  session->add_double("/c/"+id+"/psustain",&psustain);
  session->add_double_db("/c/"+id+"/gain",&gain);
  session->add_bool_true("/c/"+id+"/reset",&reset);
  session->add_bool("/c/"+id+"/active",&active_);
  session->add_bool("/c/"+id+"/bypass",&bypass_);
  // activate service:
  set_apply( 0.01f );
  activate();
}

int granularsynth_t::osc_apply(const char*, const char*, lo_arg** argv, int,
                               lo_message, void* user_data)
{
  ((granularsynth_t*)user_data)->set_apply(argv[0]->f);
  return 0;
}

void granularsynth_t::set_apply(float t)
{
  deltaw = 0;
  t_apply = 0;
  if( t >= 0 ){
    uint32_t tau(std::max(1,(int32_t)(srate*t)));
    deltaw = (wet-currentw)/(float)tau;
    t_apply = tau;
  }
}

int granularsynth_t::process(jack_nframes_t n, const std::vector<float*>& vIn, const std::vector<float*>& vOut)
{
  jackc_db_t::process(n,vIn,vOut);
  if( active_ ){
    // apply dry/wet mixing:
    TASCAR::wave_t w_in(n,vIn[0]);
    TASCAR::wave_t w_out(n,vOut[0]);
    for(uint32_t k=0;k<w_in.n;++k){
      if( t_apply ){
	t_apply--;
	currentw += deltaw;
      }
      if( currentw < 0.0f )
	currentw = 0.0f;
      if( currentw > 1.0f )
	currentw = 1.0f;
      w_out[k] *= gain*currentw;
      w_out[k] += (1.0f-currentw)*w_in[k];
    }
  }else{
    if( bypass_ ){
    // copy input to output:
      for(size_t ch=0;ch<std::min(vIn.size(),vOut.size());++ch)
	memcpy( vOut[ch], vIn[ch], n*sizeof(float) );
      for(size_t ch=vIn.size();ch<vOut.size();++ch)
	memset( vOut[ch], 0, n*sizeof(float) );
    }else{
      for(size_t ch=0;ch<vOut.size();++ch)
	memset( vOut[ch], 0, n*sizeof(float) );
    }
  }
  return 0;
}


int granularsynth_t::inner_process(jack_nframes_t n, const std::vector<float*>& vIn, const std::vector<float*>& vOut)
{
  // calculate time base:
  uint64_t t0i(t0*srate);
  uint64_t t(jack_get_current_transport_frame( jc ));
  t = t-std::min(t,t0i);
  uint64_t loopi(loop*srate/(bpm/60.0));
  ldiv_t tmp(ldiv( t, loopi ));
  t = tmp.rem;
  // if inactive, do only time update, and return zeros:
  if( !active_ ){
    tprev = t;
    for( auto it=vOut.begin();it!=vOut.end();++it)
      memset((*it),0,sizeof(float)*n);
    return 0;
  }
  // optionally, reset grains:
  if( reset ){
    for( auto it=grains.begin();it!=grains.end();++it )
      it->reset();
    reset = false;
  }
  // create delayed input signal:
  if( in_delayed.n != n ){
    DEBUG(in_delayed.n);
    DEBUG(n);
    throw TASCAR::ErrMsg("Programming error");
  }
  TASCAR::wave_t w_in(n,vIn[0]);
  for( uint32_t k=0;k<n;++k){
    in_delayed.d[k] = delaystate;
    delaystate = w_in.d[k];
  }
  // spectral analysis and instantaneous frequency:
  ola1.process(w_in);
  ola2.process(in_delayed);
  // use frequency with highest intensity:
  double intensity_max(0.0);
  double freq_max(0.0);
  for( uint32_t k=0;k<ola1.s.n_;++k ){
    std::complex<float> p(ola1.s.b[k] * std::conj(ola2.s.b[k]));
    float f(std::arg(p));
    float intensity(std::abs(p));
    if( intensity > intensity_max ){
      freq_max = f;
      intensity_max = intensity;
    }
  }
  // if instantaneous frequency matches, then add grain:
  freq_max *= f_sample/TASCAR_2PI;
  // phase modification, e.g., convert to minimum phase stimulus:
  minphase( ola1.s );
  // add grains to grain store if features are matching:
  for( auto it=grains.begin();it!=grains.end();++it )
    if( fabs(12.0*log2(freq_max/(it->f))) < 0.5 ){
      it->add( ola1.s );
    }
  // prepare for output:
  ola1.s.clear();
  TASCAR::wave_t w_out(n,vOut[0]);
  // play melody from grain collection:
  const double vrandom(TASCAR::drand());
  for( uint32_t k=0;k<std::min(startingtimes.size(),vfreqs.size());++k ){
    uint64_t tstart(startingtimes[k]);
    if( ((t >= tstart) && ((tprev < tstart) || (tprev>t))) && (vrandom < ponset) )
      for( auto it=grains.begin();it!=grains.end();++it )
	if( vfreqs[k] == it->f ){
	  it->play( ola1.s, gain );
	}
    if( (t >= tstart) && (t < tstart+idurations[k]) && (vrandom < psustain) )
      for( auto it=grains.begin();it!=grains.end();++it )
	if( vfreqs[k] == it->f ){
	  it->play( ola1.s, gain );
	}
  }
  // convert back to time domain:
  ola1.ifft(w_out);
  //w_out.clear();
  tprev = t;
  return 0;
}

granularsynth_t::~granularsynth_t()
{
  deactivate();
}

REGISTER_MODULE(granularsynth_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

