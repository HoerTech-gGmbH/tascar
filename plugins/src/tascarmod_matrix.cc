#include "session.h"
#include "jackclient.h"
#include <complex.h>

#define SQRT12 0.70710678118654757274f

class matrix_vars_t : public TASCAR::module_base_t {
public:
  matrix_vars_t( const TASCAR::module_cfg_t& cfg );
  ~matrix_vars_t();
protected:
  std::string id;
  std::string decoder;
  bool own_outputs;
  std::string outname;
};

class matrix_t : public matrix_vars_t, public jackc_t {
public:
  matrix_t( const TASCAR::module_cfg_t& cfg );
  ~matrix_t();
  void release();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
  void prepare( chunk_cfg_t& );
private:
  TASCAR::spk_array_t outputs;
  TASCAR::spk_array_t inputs;
  std::vector<std::vector<float> > m;
};

matrix_vars_t::matrix_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    own_outputs(true),
    outname("output")
{
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(decoder);
  if( has_attribute("layout") ){
    own_outputs = false;
    outname = "speaker";
  }
}

matrix_t::matrix_t( const TASCAR::module_cfg_t& cfg )
  : matrix_vars_t( cfg ),
    jackc_t(id),
    outputs( cfg.xmlsrc, own_outputs, outname ),
    inputs( cfg.xmlsrc, true, "input" )
{
  m.resize( outputs.size() );
  for(uint32_t k=0;k<outputs.size();++k)
    outputs[k].get_attribute( "m", m[k] );
  if( decoder == "maxre2d" ){
    uint32_t amborder((inputs.size()-1)/2);
    uint32_t maxc(2*amborder+1);
    for(uint32_t k=0;k<outputs.size();++k){
      m[k].resize(maxc);
      double ordergain(sqrt(0.5));
      double channelgain(1.0/maxc);
      m[k][0] = channelgain*ordergain;
        for(uint32_t o=1;o<amborder+1;++o){
          ordergain = cos(o*M_PI/(2.0*amborder+2.0));
          double _Complex cw(cpow(cexp(-I*outputs[k].azim()),o));
          // ACN!
          m[k][2*o] = channelgain*ordergain*creal(cw);
          m[k][2*o-1] = channelgain*ordergain*cimag(cw);
        }
    }
  }
  for(uint32_t k=0;k<inputs.size();++k){
    char ctmp[1024];
    if( inputs[k].label.empty() )
      sprintf(ctmp,"in.%d",k);
    else
      sprintf(ctmp,"%s",inputs[k].label.c_str());
    add_input_port(ctmp);
  }
  for(uint32_t k=0;k<outputs.size();++k){
    char ctmp[1024];
    if( outputs[k].label.empty() )
      sprintf(ctmp,"out.%d",k);
    else
      sprintf(ctmp,"%s",outputs[k].label.c_str());
    add_output_port(ctmp);
  }
  activate();
}

matrix_vars_t::~matrix_vars_t()
{
}

void matrix_t::release()
{
  module_base_t::release();
  inputs.release();
  outputs.release();
  deactivate();
}

matrix_t::~matrix_t()
{
}

void matrix_t::prepare( chunk_cfg_t& cf_ )
{
  module_base_t::prepare( cf_ );
  outputs.prepare( cf_ );
  inputs.prepare( cf_ );
  // connect output ports:
  for(uint32_t kc=0;kc<outputs.connections.size();++kc)
    if( outputs.connections[kc].size() )
      connect_out(kc,outputs.connections[kc],true);
  for(uint32_t kc=0;kc<inputs.connections.size();++kc)
    if( inputs.connections[kc].size() )
      connect_in(kc,inputs.connections[kc],true);
}

int matrix_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  for(uint32_t ko=0;ko<sOut.size();++ko){
    memset(sOut[ko],0,sizeof(float)*n);
    for(uint32_t ki=0;ki<std::min(sIn.size(),m[ko].size());++ki)
      if( m[ko][ki] != 0.0f )
        for(uint32_t k=0;k<n;++k)
          sOut[ko][k] += m[ko][ki]*sIn[ki][k];
  }
  if( outputs.delaycomp.size() == outputs.size() ){
    for( uint32_t k=0;k<outputs.size();++k){
      for( uint32_t f=0;f<n;++f ){
        sOut[k][f] = outputs[k].gain*outputs.delaycomp[k]( sOut[k][f] );
      }
    }
  }
  for( uint32_t k=0;k<outputs.size();++k){
    if( outputs[k].comp ){
      TASCAR::wave_t wOut(n,sOut[k]);
      outputs[k].comp->process(wOut,wOut,false);
    }
  }
  return 0;
}

REGISTER_MODULE(matrix_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

