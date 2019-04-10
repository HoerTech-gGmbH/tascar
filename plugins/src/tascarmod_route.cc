#include "session.h"
#include "jackclient.h"
#include "scene.h"

#define SQRT12 0.70710678118654757274f

class routemod_t : public TASCAR::module_base_t,
                   public TASCAR::Scene::route_t,
                   public jackc_t,
                   public TASCAR::Scene::audio_port_t {
public:
  routemod_t( const TASCAR::module_cfg_t& cfg );
  ~routemod_t();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
private:
  uint32_t channels;
  std::string connect_out;
};

routemod_t::routemod_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    TASCAR::Scene::route_t( TASCAR::module_base_t::e ),
    jackc_t(get_name()),
    TASCAR::Scene::audio_port_t( TASCAR::module_base_t::e, false ),
    channels(1)
{
  TASCAR::module_base_t::GET_ATTRIBUTE(channels);
  TASCAR::module_base_t::GET_ATTRIBUTE(connect_out);
  TASCAR::module_base_t::get_attribute("lingain",gain);
  session->add_float_db("/"+get_name()+"/gain",&gain);
  session->add_float("/"+get_name()+"/lingain",&gain);
  set_ctlname("/"+get_name());
  for(uint32_t k=0;k<channels;++k){
    char ctmp[1024];
    sprintf(ctmp,"in.%d",k);
    add_input_port(ctmp);
    sprintf(ctmp,"out.%d",k);
    add_output_port(ctmp);
    addmeter( get_srate() );
  }
  activate();
  std::string con(get_connect());
  if( !con.empty() ){
    std::vector<std::string> ports(get_port_names_regexp( TASCAR::env_expand(con) ) );
    if( ports.empty() )
      TASCAR::add_warning("No port \""+con+"\" found.");
    uint32_t ip(0);
    for( auto it=ports.begin();it!=ports.end();++it){
      if( ip < get_num_input_ports() ){
        connect_in( ip, *it, true, true );
        ++ip;
      }
    }
  }
  if( !connect_out.empty() ){
    std::vector<std::string> ports(get_port_names_regexp( TASCAR::env_expand(connect_out), JackPortIsInput ) );
    if( ports.empty() )
      TASCAR::add_warning("No input port \""+connect_out+"\" found.");
    uint32_t ip(0);
    for( auto it=ports.begin();it!=ports.end();++it){
      if( ip < get_num_output_ports() ){
        jackc_t::connect_out( ip, *it, true );
        ++ip;
      }
    }
  }
}

routemod_t::~routemod_t()
{
  deactivate();
}

int routemod_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  bool active(is_active(0));
  for(uint32_t ch=0;ch<std::min(sIn.size(),sOut.size());++ch){
    if( active ){
      for(uint32_t k=0;k<n;++k){
        sOut[ch][k] = gain*sIn[ch][k];
      }
    }else{
      memset(sOut[ch],0,n*sizeof(float));
    }
    rmsmeter[ch]->update( TASCAR::wave_t( n, sOut[ch] ) );
  }
  return 0;
}

REGISTER_MODULE(routemod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

