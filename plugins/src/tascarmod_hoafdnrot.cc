/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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
#include "jackclient.h"
#include <string.h>
#include <cstdlib>
#include <cmath>
#include <limits>
#include "fft.h"

const std::complex<float> i(0.0, 1.0);
const std::complex<double> i_d(0.0, 1.0);

class cmat3_t {
public:
  cmat3_t( uint32_t d1, uint32_t d2, uint32_t d3 );
  ~cmat3_t();
  inline std::complex<float>& elem(uint32_t p1,uint32_t p2,uint32_t p3) { 
    return data[p1*s23+p2*s3+p3]; 
  };
  inline const std::complex<float>& elem(uint32_t p1,uint32_t p2,uint32_t p3) const { 
    return data[p1*s23+p2*s3+p3]; 
  };
  inline std::complex<float>& elem000() { 
    return data[0]; 
  };
  inline const std::complex<float>& elem000() const { 
    return data[0]; 
  };
  inline std::complex<float>& elem00x(uint32_t p3) { 
    return data[p3]; 
  };
  inline const std::complex<float>& elem00x(uint32_t p3) const { 
    return data[p3]; 
  };
  inline void clear() {
    data.clear();
    //memset(data,0,sizeof(std::complex<float>)*s1*s2*s3);
  };
protected:
  uint32_t s1;
  uint32_t s2;
  uint32_t s3;
  uint32_t s23;
  TASCAR::spec_t data;
};

class cmat2_t {
public:
  cmat2_t( uint32_t d1, uint32_t d2 );
  ~cmat2_t();
  inline std::complex<float>& elem(uint32_t p1,uint32_t p2) { 
    return data[p1*s2+p2]; 
  };
  inline const std::complex<float>& elem(uint32_t p1,uint32_t p2) const { 
    return data[p1*s2+p2]; 
  };
  inline std::complex<float>& elem00() { 
    return data[0]; 
  };
  inline const std::complex<float>& elem00() const { 
    return data[0]; 
  };
  inline std::complex<float>& elem0x(uint32_t p2) { 
    return data[p2]; 
  };
  inline const std::complex<float>& elem0x(uint32_t p2) const { 
    return data[p2]; 
  };
  inline void clear() {
    data.clear();
    //memset(data,0,sizeof(std::complex<float>)*s1*s2);
  };
protected:
  uint32_t s1;
  uint32_t s2;
  TASCAR::spec_t data;
};

class cmat1_t {
public:
  cmat1_t( uint32_t d1 );
  ~cmat1_t();
  inline std::complex<float>& elem(uint32_t p1) { 
    return data[p1]; 
  };
  inline const std::complex<float>& elem(uint32_t p1) const { 
    return data[p1]; 
  };
  inline std::complex<float>& elem0() { 
    return data[0]; 
  };
  inline const std::complex<float>& elem0() const { 
    return data[0]; 
  };
  inline void clear() {
    data.clear();
  };
protected:
  uint32_t s1;
  TASCAR::spec_t data;
};

cmat3_t::cmat3_t( uint32_t d1, uint32_t d2, uint32_t d3 )
  : s1(d1),s2(d2),s3(d3),
    s23(s2*s3),
    data(s1*s2*s3)
{
  clear();
}

cmat3_t::~cmat3_t()
{
}

cmat2_t::cmat2_t( uint32_t d1, uint32_t d2 )
  : s1(d1),s2(d2),
    data(s1*s2)
{
  clear();
}

cmat2_t::~cmat2_t()
{
}


cmat1_t::cmat1_t( uint32_t d1 )
  : s1(d1),
    data(s1)
{
  clear();
}

cmat1_t::~cmat1_t()
{
}

//y[n] = -g x[n] + x[n-1] + g y[n-1]
class reflectionfilter_t {
public:
  reflectionfilter_t(uint32_t d1, uint32_t d2);
  inline void filter( std::complex<float>& x, uint32_t p1, uint32_t p2) {
    x = B1*x-A2*sy.elem(p1,p2);
    sy.elem(p1,p2) = x;
    // all pass section:
    std::complex<float> tmp(eta[p1]*x + sapx.elem( p1, p2 ));
    sapx.elem( p1, p2 ) = x;
    x = tmp - eta[p1]*sapy.elem( p1, p2 );
    sapy.elem( p1, p2 ) = x;
  };
  void set_lp( float g, float c );
protected:
  float B1;
  float A2;
  std::vector<float> eta;
  cmat2_t sy;
  cmat2_t sapx;
  cmat2_t sapy;
};

reflectionfilter_t::reflectionfilter_t(uint32_t d1, uint32_t d2)
  : B1(0),A2(0),
    sy(d1,d2),
    sapx(d1,d2),
    sapy(d1,d2)
{
  eta.resize(d1);
  for(uint32_t k=0;k<d1;++k)
    eta[k] = 0.87*(double)k/(d1-1);
}

void reflectionfilter_t::set_lp( float g, float c )
{
  sy.clear();
  sapx.clear();
  sapy.clear();
  float c2(1.0f-c);
  B1 = g * c2;
  A2 = -c;
}

class fdn_t {
public:
  fdn_t(uint32_t fdnorder, uint32_t amborder, uint32_t maxdelay, bool logdelays );
  ~fdn_t();
  inline void process( bool b_prefilt );
  void setpar( float az, float daz, float t, float dt, float g, float damping );
  void set_logdelays( bool ld ){ logdelays_ = ld; };
private:
  bool logdelays_;
  uint32_t fdnorder_;
  uint32_t amborder1;
  uint32_t maxdelay_;
  // delayline:
  cmat3_t delayline;
  // feedback matrix:
  cmat3_t feedbackmat;
  // reflection filter:
  reflectionfilter_t reflection;
  reflectionfilter_t prefilt;
  // rotation:
  cmat2_t rotation;
  // delayline output for reflection filters:
  cmat2_t dlout;
  // delays:
  uint32_t* delay;
  // delayline pointer:
  uint32_t* pos;
public:
  // input HOA sample:
  cmat1_t inval;
  // output HOA sample:
  cmat1_t outval;
};

fdn_t::fdn_t(uint32_t fdnorder, uint32_t amborder, uint32_t maxdelay, bool logdelays )
  : logdelays_(logdelays),
    fdnorder_(fdnorder),
    amborder1(amborder+1),
    maxdelay_(maxdelay),
    delayline(fdnorder_,maxdelay_,amborder1),
    feedbackmat(fdnorder_,fdnorder_,amborder1),
    reflection(fdnorder,amborder1),
    prefilt(2,amborder1),
    rotation(fdnorder,amborder1),
    dlout(fdnorder_,amborder1),
    delay(new uint32_t[fdnorder_]),
    pos(new uint32_t[fdnorder_]),
    inval(amborder1),
    outval(amborder1)
{
  //DEBUG(fdnorder);
  memset(delay,0,sizeof(uint32_t)*fdnorder_);
  memset(pos,0,sizeof(uint32_t)*fdnorder_);
}

fdn_t::~fdn_t()
{
  delete [] delay;
  delete [] pos;
}

void fdn_t::process(bool b_prefilt)
{
  if( b_prefilt ){
    for(uint32_t o=0;o<amborder1;++o){
      prefilt.filter(inval.elem(o),0,o);
      prefilt.filter(inval.elem(o),1,o);
    }
  }
  outval.clear();
  // get output values from delayline, apply reflection filters and rotation:
  for(uint32_t tap=0;tap<fdnorder_;++tap)
    for(uint32_t o=0;o<amborder1;++o){
      std::complex<float> tmp(delayline.elem(tap,pos[tap],o));
      reflection.filter(tmp,tap,o);
      tmp *= rotation.elem(tap,o);
      dlout.elem(tap,o) = tmp;
      outval.elem(o) += tmp;
    }
  // put rotated+attenuated value to delayline, add input:
  for(uint32_t tap=0;tap<fdnorder_;++tap){
    // first put input into delayline:
    for(uint32_t o=0;o<amborder1;++o)
      delayline.elem(tap,pos[tap],o) = inval.elem(o);
    // now add feedback signal:
    for(uint32_t otap=0;otap<fdnorder_;++otap)
      for(uint32_t o=0;o<amborder1;++o)
        delayline.elem(tap,pos[tap],o) += dlout.elem(otap,o)*feedbackmat.elem(tap,otap,o);
    // iterate delayline:
    if( !pos[tap] )
      pos[tap] = delay[tap];
    if( pos[tap] )
      --pos[tap];
  }
}

/**
   \brief Set parameters of FDN
   \param az Average rotation in radians per reflection
   \param daz Spread of rotation in radians per reflection
   \param t Average/maximum delay in samples
   \param dt Spread of delay in samples
   \param g Gain
   \param damping Damping
 */
void fdn_t::setpar(float az, float daz, float t, float dt, float g, float damping )
{
  // set reflection filters:
  reflection.set_lp( g, damping );
  prefilt.set_lp( g, damping );
  //reflection.set( g );
  // set delays:
  delayline.clear();
  for(uint32_t tap=0;tap<fdnorder_;++tap){
    double t_(t);
    if( logdelays_ ){
      if( fdnorder_ > 1 )
        t_ = dt*pow(t/dt,(double)tap/((double)fdnorder_-1.0));;
    }else{
      if( fdnorder_ > 1 )
        t_ = t-dt+2.0*dt*pow(tap*1.0/(fdnorder_),0.5);
    }
    uint32_t d(std::max(0.0,t_));
    delay[tap] = std::max(2u,std::min(maxdelay_-1u,d));
  }
  // set rotation:
  for(uint32_t tap=0;tap<fdnorder_;++tap){
    float laz(az);
    if( fdnorder_ > 1 )
      laz = az-daz+2.0*daz*tap*1.0/(fdnorder_);
    std::complex<float> caz(std::exp(i*laz));
    rotation.elem(tap,0) = 1.0;
    for(uint32_t o=1;o<amborder1;++o){
      rotation.elem(tap,o) = rotation.elem(tap,o-1)*caz;
    }  
  }
  // set feedback matrix:
  feedbackmat.clear();
  if( fdnorder_ > 1 ){
    TASCAR::fft_t fft(fdnorder_);
    TASCAR::spec_t eigenv(fdnorder_/2+1);
    for(uint32_t k=0;k<eigenv.n_;++k)
      eigenv[k] = std::exp(i_d*TASCAR_2PI*pow((double)k/(0.5*fdnorder_),2.0));;
    fft.execute(eigenv);
    //std::cout << "row: " << fft.w << std::endl;
    for(uint32_t itap=0;itap<fdnorder_;++itap){
      for(uint32_t otap=0;otap<fdnorder_;++otap){
        feedbackmat.elem(itap,otap,0) = fft.w[(otap+itap) % fdnorder_];
        for(uint32_t o=1;o<amborder1;++o)
          feedbackmat.elem(itap,otap,o) = feedbackmat.elem(itap,otap,o-1);
      }
    }
  }else{
    for(uint32_t o=0;o<amborder1;++o)
      feedbackmat.elem00x(o) = 1.0;
  }
}
  
class hoafdnrot_vars_t : public TASCAR::module_base_t {
public:
  hoafdnrot_vars_t( const TASCAR::module_cfg_t& cfg );
  ~hoafdnrot_vars_t();
protected:
  std::string id;
  uint32_t amborder;
  uint32_t fdnorder;
  double w;
  double dw;
  double t;
  double dt;
  double decay;
  double damping;
  double dry;
  double wet;
  bool prefilt;
  bool logdelays;
};

hoafdnrot_vars_t::hoafdnrot_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    amborder(3),
    fdnorder(5),
    w(1.0),
    dw(0.1),
    t(0.01),
    dt(0.002),
    decay(1.0),
    damping(0.3),
    dry(0.0),
    wet(1.0),
    prefilt(false),
    logdelays(false)
{
  GET_ATTRIBUTE_(id);
  GET_ATTRIBUTE_(amborder);
  GET_ATTRIBUTE_(fdnorder);
  GET_ATTRIBUTE_(w);
  GET_ATTRIBUTE_(dw);
  GET_ATTRIBUTE_(t);
  GET_ATTRIBUTE_(dt);
  GET_ATTRIBUTE_(decay);
  GET_ATTRIBUTE_(damping);
  GET_ATTRIBUTE_(dry);
  GET_ATTRIBUTE_(wet);
  GET_ATTRIBUTE_BOOL_(prefilt);
  GET_ATTRIBUTE_BOOL_(logdelays);
}

hoafdnrot_vars_t::~hoafdnrot_vars_t()
{
}

class hoafdnrot_t : public hoafdnrot_vars_t, public jackc_t {
public:
  hoafdnrot_t( const TASCAR::module_cfg_t& cfg );
  ~hoafdnrot_t();
  void set_par( double w, double dw, double t, double dt, double g, double damping );
  static int osc_setpar(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setlogdelays( bool ld );
  static int osc_setlogdelays(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
  void configure( );
private:
  uint32_t channels;
  fdn_t* fdn;
  uint32_t o1;
  pthread_mutex_t mtx;
};

hoafdnrot_t::hoafdnrot_t( const TASCAR::module_cfg_t& cfg )
  : hoafdnrot_vars_t( cfg ),
    jackc_t(id),
    channels(amborder*2+1),
    fdn(NULL),
    o1(amborder+1)
{
  pthread_mutex_init( &mtx, NULL );
  for(uint32_t c=0;c<channels;++c){
    char ctmp[1024];
    uint32_t o((c+1)/2);
    int32_t s(o*(2*((c+1) % 2)-1));
    sprintf(ctmp,"in.%d_%d",o,s);
    add_input_port(ctmp);
  }
  for(uint32_t c=0;c<channels;++c){
    char ctmp[1024];
    uint32_t o((c+1)/2);
    int32_t s(o*(2*((c+1) % 2)-1));
    sprintf(ctmp,"out.%d_%d",o,s);
    add_output_port(ctmp);
  }
  session->add_method("/"+id+"/par","ffffff",&hoafdnrot_t::osc_setpar,this);
  session->add_double("/"+id+"/dry",&dry);
  session->add_double("/"+id+"/wet",&wet);
  session->add_bool("/"+id+"/prefilt",&prefilt);
  session->add_method("/"+id+"/logdelays","i",&hoafdnrot_t::osc_setlogdelays,this);
  activate();
}

int hoafdnrot_t::osc_setpar(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc==6) && (types[0]=='f') )
    ((hoafdnrot_t*)user_data)->set_par( argv[0]->f, argv[1]->f, argv[2]->f, argv[3]->f, argv[4]->f, argv[5]->f );
  return 0;
}

int hoafdnrot_t::osc_setlogdelays(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc==1) && (types[0]=='i') )
    ((hoafdnrot_t*)user_data)->setlogdelays( argv[0]->i );
  return 0;
}

void hoafdnrot_t::configure( )
{
  module_base_t::configure( );
  if( fdn )
    delete fdn;
  fdn = new fdn_t(fdnorder,amborder,f_sample, logdelays );
  set_par( w, dw, t, dt, decay, damping );
}

hoafdnrot_t::~hoafdnrot_t()
{
  deactivate();
  delete fdn;
  pthread_mutex_destroy(&mtx);
}

void hoafdnrot_t::set_par( double w_, double dw_, double t_, double dt_, double decay_, double damping_ )
{
  w = w_;
  dw = dw_;
  t = t_;
  dt = dt_;
  decay = decay_;
  damping = damping_;
  if( pthread_mutex_lock( &mtx ) == 0 ){
    if( fdn ){
      const double wscale(TASCAR_2PI*t);
      fdn->setpar(wscale*w,wscale*dw,f_sample*t,f_sample*dt,exp(-t/decay),std::max(0.0,std::min(0.999,damping)));
    }
    pthread_mutex_unlock( &mtx);
  }
}

void hoafdnrot_t::setlogdelays( bool ld )
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    if( fdn ){
      fdn->set_logdelays(ld);
      const double wscale(TASCAR_2PI*t);
      fdn->setpar(wscale*w,wscale*dw,f_sample*t,f_sample*dt,exp(-t/decay),std::max(0.0,std::min(0.999,damping)));
    }
    pthread_mutex_unlock( &mtx);
  }
}

int hoafdnrot_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  if( pthread_mutex_trylock( &mtx ) == 0 ){
    if( fdn ){
      for(uint32_t c=0;c<channels;++c)
        for( uint32_t t=0;t<n;t++)
          sOut[c][t] = dry*sIn[c][t];
      for( uint32_t t=0;t<n;t++){
        fdn->inval.elem0() = sIn[0][t];
        for(uint32_t o=1;o<o1;++o)
          // ACN!
          fdn->inval.elem(o) = sIn[2*o][t]+sIn[2*o-1][t]*i;
        fdn->process(prefilt);
        sOut[0][t] += wet*fdn->outval.elem0().real();
        for(uint32_t o=1;o<o1;++o){
          // ACN!
          sOut[2*o][t] += wet*fdn->outval.elem(o).real();
          sOut[2*o-1][t] += wet*fdn->outval.elem(o).imag();
        }
      }
    }
    pthread_mutex_unlock( &mtx);
  }
  return 0;
}

REGISTER_MODULE(hoafdnrot_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

