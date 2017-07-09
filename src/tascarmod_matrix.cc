#include "session.h"
#include "jackclient.h"

#define SQRT12 0.70710678118654757274f

class matrix_vars_t : public TASCAR::module_base_t {
public:
  matrix_vars_t( const TASCAR::module_cfg_t& cfg );
  ~matrix_vars_t();
protected:
  std::string id;
};

class matrix_t : public matrix_vars_t, public jackc_t {
public:
  matrix_t( const TASCAR::module_cfg_t& cfg );
  ~matrix_t();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
  void configure(double srate,uint32_t fragsize);
private:
  TASCAR::spk_array_t outputs;
  TASCAR::spk_array_t inputs;
  std::vector<std::vector<float> > m;
};

matrix_vars_t::matrix_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg )
{
  GET_ATTRIBUTE(id);
}

matrix_t::matrix_t( const TASCAR::module_cfg_t& cfg )
  : matrix_vars_t( cfg ),
    jackc_t(id),
    outputs( cfg.xmlsrc ),
    inputs( cfg.xmlsrc, "input" )
{
  m.resize( outputs.size() );
  for(uint32_t k=0;k<inputs.size();++k){
    char ctmp[1024];
    sprintf(ctmp,"in.%d%s",k,inputs[k].label.c_str());
    add_input_port(ctmp);
  }
  for(uint32_t k=0;k<outputs.size();++k){
    char ctmp[1024];
    sprintf(ctmp,"out.%d%s",k,outputs[k].label.c_str());
    add_output_port(ctmp);
    outputs[k].get_attribute( "m", m[k] );
  }
  activate();
}

matrix_vars_t::~matrix_vars_t()
{
}

matrix_t::~matrix_t()
{
  deactivate();
}

void matrix_t::configure(double srate,uint32_t fragsize)
{
  outputs.configure( srate, fragsize );
  inputs.configure( srate, fragsize );
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
    if( outputs[k].comp )
      outputs[k].comp->filter(sOut[k],sOut[k],n,1);
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

