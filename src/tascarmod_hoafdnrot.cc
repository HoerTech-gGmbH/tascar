#include "session.h"
#include "jackclient.h"
#include <string.h>
#include <complex.h>
#include <cstdlib>
#include <cmath>
#include <limits>
#include "fft.h"

class cmat3_t {
public:
  cmat3_t( uint32_t d1, uint32_t d2, uint32_t d3 );
  ~cmat3_t();
  inline float _Complex& elem(uint32_t p1,uint32_t p2,uint32_t p3) { 
    return data[p1*s23+p2*s3+p3]; 
  };
  inline const float _Complex& elem(uint32_t p1,uint32_t p2,uint32_t p3) const { 
    return data[p1*s23+p2*s3+p3]; 
  };
  inline float _Complex& elem000() { 
    return data[0]; 
  };
  inline const float _Complex& elem000() const { 
    return data[0]; 
  };
  inline float _Complex& elem00x(uint32_t p3) { 
    return data[p3]; 
  };
  inline const float _Complex& elem00x(uint32_t p3) const { 
    return data[p3]; 
  };
  inline void clear() { memset(data,0,sizeof(float _Complex)*s1*s2*s3); };
protected:
  uint32_t s1;
  uint32_t s2;
  uint32_t s3;
  uint32_t s23;
  float _Complex* data;
};

class cmat2_t {
public:
  cmat2_t( uint32_t d1, uint32_t d2 );
  ~cmat2_t();
  inline float _Complex& elem(uint32_t p1,uint32_t p2) { 
    return data[p1*s2+p2]; 
  };
  inline const float _Complex& elem(uint32_t p1,uint32_t p2) const { 
    return data[p1*s2+p2]; 
  };
  inline float _Complex& elem00() { 
    return data[0]; 
  };
  inline const float _Complex& elem00() const { 
    return data[0]; 
  };
  inline float _Complex& elem0x(uint32_t p2) { 
    return data[p2]; 
  };
  inline const float _Complex& elem0x(uint32_t p2) const { 
    return data[p2]; 
  };
  inline void clear() { memset(data,0,sizeof(float _Complex)*s1*s2); };
protected:
  uint32_t s1;
  uint32_t s2;
  float _Complex* data;
};

class cmat1_t {
public:
  cmat1_t( uint32_t d1 );
  ~cmat1_t();
  inline float _Complex& elem(uint32_t p1) { 
    return data[p1]; 
  };
  inline const float _Complex& elem(uint32_t p1) const { 
    return data[p1]; 
  };
  inline float _Complex& elem0() { 
    return data[0]; 
  };
  inline const float _Complex& elem0() const { 
    return data[0]; 
  };
  inline void clear() { memset(data,0,sizeof(float _Complex)*s1); };
protected:
  uint32_t s1;
  float _Complex* data;
};

cmat3_t::cmat3_t( uint32_t d1, uint32_t d2, uint32_t d3 )
  : s1(d1),s2(d2),s3(d3),
    s23(s2*s3),
    data(new float _Complex[s1*s2*s3])
{
  clear();
}

cmat3_t::~cmat3_t()
{
  delete [] data;
}

cmat2_t::cmat2_t( uint32_t d1, uint32_t d2 )
  : s1(d1),s2(d2),
    data(new float _Complex[s1*s2])
{
  clear();
}

cmat2_t::~cmat2_t()
{
  delete [] data;
}


cmat1_t::cmat1_t( uint32_t d1 )
  : s1(d1),
    data(new float _Complex[s1])
{
  clear();
}

cmat1_t::~cmat1_t()
{
  delete [] data;
}

//y[n] = -g x[n] + x[n-1] + g y[n-1]
class reflectionfilter_t {
public:
  reflectionfilter_t(uint32_t d1, uint32_t d2);
  inline void filter( float _Complex& x, uint32_t p1, uint32_t p2) {
    x = B1*x-A2*sy.elem(p1,p2);
    sy.elem(p1,p2) = x;
    // all pass section:
    float _Complex tmp(eta[p1]*x + sapx.elem( p1, p2 ));
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
  fdn_t(uint32_t fdnorder, uint32_t amborder, uint32_t maxdelay );
  ~fdn_t();
  inline void process( bool b_prefilt );
  void setpar( float az, float daz, float t, float dt, float g, float damping );
private:
  uint32_t fdnorder_;
  uint32_t amborder1;
  uint32_t maxdelay_;
  uint32_t taplen;
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

fdn_t::fdn_t(uint32_t fdnorder, uint32_t amborder, uint32_t maxdelay)
  : fdnorder_(fdnorder),
    amborder1(amborder+1),
    maxdelay_(maxdelay),
    taplen(maxdelay*amborder1),
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
      float _Complex tmp(delayline.elem(tap,pos[tap],o));
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
    if( fdnorder_ > 1 )
      t_ = t-dt+2.0*dt*pow(tap*1.0/(fdnorder_),0.5);
    uint32_t d(std::max(0.0,t_));
    delay[tap] = std::max(2u,std::min(maxdelay_-1u,d));
  }
  // set rotation:
  for(uint32_t tap=0;tap<fdnorder_;++tap){
    float laz(az);
    if( fdnorder_ > 1 )
      laz = az-daz+2.0*daz*tap*1.0/(fdnorder_);
    float _Complex caz(cexpf(I*laz));
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
      eigenv[k] = cexp(I*2.0*M_PI*pow((double)k/(0.5*fdnorder_),2.0));;
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
    prefilt(false)
{
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(amborder);
  GET_ATTRIBUTE(fdnorder);
  GET_ATTRIBUTE(w);
  GET_ATTRIBUTE(dw);
  GET_ATTRIBUTE(t);
  GET_ATTRIBUTE(dt);
  GET_ATTRIBUTE(decay);
  GET_ATTRIBUTE(damping);
  GET_ATTRIBUTE(dry);
  GET_ATTRIBUTE(wet);
  GET_ATTRIBUTE_BOOL(prefilt);
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
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
  void prepare( chunk_cfg_t& );
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
  activate();
}

int hoafdnrot_t::osc_setpar(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc==6) && (types[0]=='f') )
    ((hoafdnrot_t*)user_data)->set_par( argv[0]->f, argv[1]->f, argv[2]->f, argv[3]->f, argv[4]->f, argv[5]->f );
  return 0;
}

void hoafdnrot_t::prepare( chunk_cfg_t& cf_ )
{
  module_base_t::prepare( cf_ );
  if( fdn )
    delete fdn;
  fdn = new fdn_t(fdnorder,amborder,f_sample);
  set_par( w, dw, t, dt, decay, damping );
}

hoafdnrot_t::~hoafdnrot_t()
{
  deactivate();
  delete fdn;
  pthread_mutex_destroy(&mtx);
}

void hoafdnrot_t::set_par( double w, double dw, double t, double dt, double decay, double damping )
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    if( fdn ){
      double wscale(2.0*M_PI*t);
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
          fdn->inval.elem(o) = sIn[2*o][t]+sIn[2*o-1][t]*I;
        fdn->process(prefilt);
        sOut[0][t] += wet*creal(fdn->outval.elem0());
        for(uint32_t o=1;o<o1;++o){
          // ACN!
          sOut[2*o][t] += wet*creal(fdn->outval.elem(o));
          sOut[2*o-1][t] += wet*cimag(fdn->outval.elem(o));
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

