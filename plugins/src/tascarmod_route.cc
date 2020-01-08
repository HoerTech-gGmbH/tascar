#include "session.h"
#include "jackclient.h"
#include "scene.h"
#include "pluginprocessor.h"

#define SQRT12 0.70710678118654757274f

class routemod_t : public TASCAR::module_base_t,
                   public TASCAR::Scene::route_t,
                   public jackc_transport_t,
                   public TASCAR::Scene::audio_port_t {
public:
  routemod_t( const TASCAR::module_cfg_t& cfg );
  ~routemod_t();
  virtual int process(jack_nframes_t,
                      const std::vector<float*>&,
                      const std::vector<float*>&, 
                      uint32_t tp_frame, bool tp_rolling);
  void prepare( chunk_cfg_t& cf );
  void release( );
private:
  uint32_t channels;
  std::vector<std::string> connect_out;
  double levelmeter_tc;
  TASCAR::levelmeter::weight_t levelmeter_weight;
  TASCAR::plugin_processor_t plugins;
  TASCAR::pos_t nullpos;
  TASCAR::zyx_euler_t nullrot;
  float dummy;
  std::vector<TASCAR::wave_t> sIn_tsc;
  bool bypass;
  pthread_mutex_t mtx_;
};

int osc_routemod_mute(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  routemod_t* h((routemod_t*)user_data);
  if( h && (argc == 1) && (types[0]=='i') ){
    h->set_mute(argv[0]->i);
    return 0;
  }
  return 1;
}

routemod_t::routemod_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    TASCAR::Scene::route_t( TASCAR::module_base_t::e ),
    jackc_transport_t(get_name()),
    TASCAR::Scene::audio_port_t( TASCAR::module_base_t::e, false ),
  channels(1),
  levelmeter_tc(2.0),
  levelmeter_weight(TASCAR::levelmeter::Z),
  plugins( TASCAR::module_base_t::e, get_name(), "" ),
  bypass(true)
{
  pthread_mutex_init(&mtx_,NULL);
  TASCAR::module_base_t::GET_ATTRIBUTE(channels);
  TASCAR::module_base_t::GET_ATTRIBUTE(connect_out);
  TASCAR::module_base_t::get_attribute("lingain",gain);
  TASCAR::module_base_t::GET_ATTRIBUTE(levelmeter_tc);
  TASCAR::module_base_t::GET_ATTRIBUTE(levelmeter_weight);
  session->add_float_db("/"+get_name()+"/gain",&gain);
  session->add_float("/"+get_name()+"/lingain",&gain);
  session->add_method("/"+get_name()+"/mute","i",osc_routemod_mute,this);
  configure_meter( levelmeter_tc, levelmeter_weight );
  set_ctlname("/"+get_name());
  for(uint32_t k=0;k<channels;++k){
    char ctmp[1024];
    sprintf(ctmp,"in.%d",k);
    add_input_port(ctmp);
    sprintf(ctmp,"out.%d",k);
    add_output_port(ctmp);
    addmeter( get_srate() );
  }
  std::string pref(session->get_prefix());
  session->set_prefix("/"+get_name());
  plugins.add_variables( session );
  plugins.add_licenses( session );
  session->set_prefix(pref);
  activate();
}

void routemod_t::prepare( chunk_cfg_t& cf )
{
  pthread_mutex_lock( &mtx_ );
  cf.n_channels =  channels;
  sIn_tsc.clear();
  for( uint32_t ch=0;ch<channels;++ch)
    sIn_tsc.push_back( TASCAR::wave_t( fragsize ) );
  plugins.prepare(cf);
  TASCAR::module_base_t::prepare(cf);
  TASCAR::Scene::route_t::prepare(cf);
  std::vector<std::string> con(get_connect());
  for( auto it=con.begin();it!=con.end();++it)
    *it = TASCAR::env_expand(*it);
  if( !con.empty() ){
    std::vector<std::string> ports( get_port_names_regexp( con ) );
    if( ports.empty() )
      TASCAR::add_warning("No port \""+TASCAR::vecstr2str(con)+"\" found.");
    uint32_t ip(0);
    for( auto it=ports.begin();it!=ports.end();++it){
      if( ip < get_num_input_ports() ){
        connect_in( ip, *it, true, true );
        ++ip;
      }
    }
  }
  for( auto it=connect_out.begin();it!=connect_out.end();++it)
    *it = TASCAR::env_expand(*it);
  if( !connect_out.empty() ){
    std::vector<std::string> ports(get_port_names_regexp( connect_out, JackPortIsInput ) );
    if( ports.empty() )
      TASCAR::add_warning("No input port matches \""+TASCAR::vecstr2str(connect_out)+"\".");
    uint32_t ip(0);
    for( auto it=ports.begin();it!=ports.end();++it){
      if( ip < get_num_output_ports() ){
        jackc_t::connect_out( ip, *it, true );
        ++ip;
      }
    }
  }
  bypass = false;
  pthread_mutex_unlock( &mtx_ );
}

void routemod_t::release( )
{
  pthread_mutex_lock( &mtx_ );
  bypass = true;
  TASCAR::module_base_t::release();
  TASCAR::Scene::route_t::release();
  plugins.release();
  sIn_tsc.clear();
  pthread_mutex_unlock( &mtx_ );
}

routemod_t::~routemod_t()
{
  deactivate();
  pthread_mutex_destroy(&mtx_);
}

int routemod_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut, 
                        uint32_t tp_frame, bool tp_rolling)
{
  if( bypass )
    return 0;
  if( pthread_mutex_trylock(&mtx_) == 0 ){
    TASCAR::transport_t tp;
    tp.rolling = tp_rolling;
    tp.session_time_samples = tp_frame;
    tp.session_time_seconds = (double)tp_frame/(double)srate;
    tp.object_time_samples = tp_frame;
    tp.object_time_seconds = (double)tp_frame/(double)srate;
    bool active(is_active(0));
    for(uint32_t ch=0;ch<std::min(sIn.size(),sIn_tsc.size());++ch){
      sIn_tsc[ch].copy(sIn[ch],n);
    }
    if( active )
      plugins.process_plugins( sIn_tsc, nullpos, nullrot, tp );
    for(uint32_t ch=0;ch<std::min(sIn.size(),sOut.size());++ch){
      if( active ){
        for(uint32_t k=0;k<n;++k){
          sOut[ch][k] = gain*sIn_tsc[ch].d[k];
        }
      }else{
        memset(sOut[ch],0,n*sizeof(float));
      }
      rmsmeter[ch]->update( TASCAR::wave_t( n, sOut[ch] ) );
    }
    pthread_mutex_unlock(&mtx_);
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

