#include "session.h"
#include "jackclient.h"
#include <string.h>
#include <complex.h>
#include <cstdlib>
#include <cmath>
#include <limits>
#include "fft.h"

double drand_( double mu, double rg )
{
  double r(rand()*(1.0/RAND_MAX));
  return 2.0*(r-0.5)*rg+mu;
}

class cmat3_t {
public:
  cmat3_t( uint32_t d1, uint32_t d2, uint32_t d3 );
  ~cmat3_t();
  inline float _Complex& elem(uint32_t p1,uint32_t p2,uint32_t p3) { 
    if( (p1 >= s1) || (p2 >= s2) || (p3 >= s3) ){
      //DEBUG(this);
      //DEBUG(p1);
      //DEBUG(s1);
      //DEBUG(p2);
      //DEBUG(s2);
      //DEBUG(p3);
      //DEBUG(s3);
      throw TASCAR::ErrMsg("Programming error: Index out of range!");
    }
    return data[p1*s23+p2*s3+p3]; 
  };
  inline const float _Complex& elem(uint32_t p1,uint32_t p2,uint32_t p3) const { return data[p1*s23+p2*s3+p3]; };
  inline void clear() { memset(data,0,sizeof(float _Complex)*s1*s2*s3); };
protected:
  uint32_t s1;
  uint32_t s2;
  uint32_t s3;
  uint32_t s23;
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

class reflectionfilter_t {
public:
  reflectionfilter_t(uint32_t d1, uint32_t d2);
  inline void filter( float _Complex& x, uint32_t p1, uint32_t p2) {
    float _Complex tmp(B1*x);
    tmp += B2*sx.elem(p1,0,p2);
    sx.elem(p1,0,p2) = x;
    x = tmp-A2*sy.elem(p1,0,p2);
    //x *= A1;
    sy.elem(p1,0,p2) = x;
  };
  // to be replaced by 1st order IIR filter!
  void set( float g );
  void set_lp( float g, float c );
protected:
  float B1;
  float B2;
  //float A1;
  float A2;
  cmat3_t sx;
  cmat3_t sy;
};

reflectionfilter_t::reflectionfilter_t(uint32_t d1, uint32_t d2)
  : B1(0),B2(0),A2(0),
    sx(d1,1,d2),
    sy(d1,1,d2)
{
}

void reflectionfilter_t::set( float g )
{
  sx.clear();
  sy.clear();
  B1 = g;
  B2 = 0.0f;
  //A1 = 1.0f;
  A2 = 0.0f;
}

void reflectionfilter_t::set_lp( float g, float c )
{
  sx.clear();
  sy.clear();
  float c2(1.0f-c);
  B1 = g * c2;
  B2 = 0.0f;
  //A1 = 1.0f;
  A2 = -c;
}

class fdn_t {
public:
  fdn_t(uint32_t fdnorder, uint32_t amborder, uint32_t maxdelay);
  ~fdn_t();
  void process(); 
  void setpar(float az, float daz, float t, float dt, float g, float damping );
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
  // rotation:
  cmat3_t rotation;
  // delayline output for reflection filters:
  cmat3_t dlout;
  // delays:
  uint32_t* delay;
  // delayline pointer:
  uint32_t* pos;
public:
  // input HOA sample:
  cmat3_t inval;
  // output HOA sample:
  cmat3_t outval;
};

fdn_t::fdn_t(uint32_t fdnorder, uint32_t amborder, uint32_t maxdelay)
  : fdnorder_(fdnorder),
    amborder1(amborder+1),
    maxdelay_(maxdelay),
    taplen(maxdelay*amborder1),
    delayline(fdnorder_,maxdelay_,amborder1),
    feedbackmat(fdnorder_,fdnorder_,amborder1),
    reflection(fdnorder,amborder1),
    rotation(fdnorder,1,amborder1),
    dlout(fdnorder_,1,amborder1),
    delay(new uint32_t[fdnorder_]),
    pos(new uint32_t[fdnorder_]),
    inval(1,1,amborder1),
    outval(1,1,amborder1)
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

void fdn_t::process()
{
  outval.clear();
  // get output values from delayline, apply reflection filters and rotation:
  for(uint32_t tap=0;tap<fdnorder_;++tap)
    for(uint32_t o=0;o<amborder1;++o){
      float _Complex tmp(delayline.elem(tap,pos[tap],o));
      reflection.filter(tmp,tap,o);
      tmp *= rotation.elem(tap,0,o);
      dlout.elem(tap,0,o) = tmp;
      outval.elem(0,0,o) += tmp;
    }
  // put rotated+attenuated value to delayline, add input:
  for(uint32_t tap=0;tap<fdnorder_;++tap){
    // first put input into delayline:
    for(uint32_t o=0;o<amborder1;++o)
      delayline.elem(tap,pos[tap],o) = inval.elem(0,0,o);
    // now add feedback signal:
    for(uint32_t otap=0;otap<fdnorder_;++otap)
      for(uint32_t o=0;o<amborder1;++o)
        delayline.elem(tap,pos[tap],o) += dlout.elem(otap,0,o)*feedbackmat.elem(tap,otap,o);
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
    rotation.elem(tap,0,0) = 1.0;
    for(uint32_t o=1;o<amborder1;++o){
      rotation.elem(tap,0,o) = rotation.elem(tap,0,o-1)*caz;
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
      feedbackmat.elem(0,0,o) = 1.0;
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
    dry(0.0)
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
  void configure(double srate,uint32_t fragsize);
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
    //sprintf(ctmp,"out.%d_%d",o,s);
    //add_output_port(ctmp);
  }
  for(uint32_t c=0;c<channels;++c){
    char ctmp[1024];
    uint32_t o((c+1)/2);
    int32_t s(o*(2*((c+1) % 2)-1));
    //sprintf(ctmp,"in.%d_%d",o,s);
    //add_input_port(ctmp);
    sprintf(ctmp,"out.%d_%d",o,s);
    add_output_port(ctmp);
  }
  session->add_method("/"+id+"/par","ffffff",&hoafdnrot_t::osc_setpar,this);
  session->add_double("/"+id+"/dry",&dry);
  activate();
}

int hoafdnrot_t::osc_setpar(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc==6) && (types[0]=='f') )
    ((hoafdnrot_t*)user_data)->set_par( argv[0]->f, argv[1]->f, argv[2]->f, argv[3]->f, argv[4]->f, argv[5]->f );
  return 0;
}

void hoafdnrot_t::configure(double srate,uint32_t fragsize)
{
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
        fdn->inval.elem(0,0,0) = sIn[0][t];
        for(uint32_t o=1;o<o1;++o)
          // ACN!
          fdn->inval.elem(0,0,o) = sIn[2*o][t]+sIn[2*o-1][t]*I;
        fdn->process();
        sOut[0][t] += creal(fdn->outval.elem(0,0,0));
        for(uint32_t o=1;o<o1;++o){
          // ACN!
          sOut[2*o][t] += creal(fdn->outval.elem(0,0,o));
          sOut[2*o-1][t] += cimag(fdn->outval.elem(0,0,o));
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

