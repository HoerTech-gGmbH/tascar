#include "session.h"
#include <lsl_cpp.h>
#include <thread>

class lslactor_t : public TASCAR::actor_module_t {
public:
  lslactor_t( const TASCAR::module_cfg_t& cfg );
  ~lslactor_t();
  void update(uint32_t frame, bool running);
  void service();
private:
  std::string predicate;
  std::vector<int32_t> channels;
  std::vector<double> influence;
  lsl::stream_inlet* inlet;
  std::thread srv;
  bool run_service;
  TASCAR::c6dof_t transform;
  bool local;
  bool incremental;
};

lslactor_t::lslactor_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg ),
    inlet(NULL),
    run_service(true),
    local(false),
    incremental(false)
{
  GET_ATTRIBUTE(predicate);
  GET_ATTRIBUTE(channels);
  GET_ATTRIBUTE(influence);
  GET_ATTRIBUTE_BOOL(local);
  GET_ATTRIBUTE_BOOL(incremental);
  if( channels.empty() )
    throw TASCAR::ErrMsg("No channels selected.");
  if( channels.size() > 6 )
    throw TASCAR::ErrMsg("More than 6 channels selected.");
  for(uint32_t k=channels.size();k<6;++k)
    channels.push_back(-1);
  influence.resize(channels.size());
  bool valid(false);
  for(uint32_t k=0;k<channels.size();++k)
    if( (channels[k]>-1) && (influence[k]!=0) )
      valid = true;
  if( !valid )
    throw TASCAR::ErrMsg("No channel has a non-zero influence.");
  std::vector<lsl::stream_info> results(lsl::resolve_stream(predicate,1,1));
  if( results.empty() )
    throw TASCAR::ErrMsg("No matching LSL stream found ("+predicate+").");
  lsl::channel_format_t cf(results[0].channel_format());
  if( !((cf == lsl::cf_float32) || (cf == lsl::cf_double64)) )
    throw TASCAR::ErrMsg("The LSL stream \""+predicate+"\" has no floating point data.");
  int32_t streamchannels(results[0].channel_count());
  for(uint32_t k=0;k<channels.size();++k)
    if( (channels[k] > -1) && (channels[k]>=streamchannels) ){
      char ctmp[1024];
      sprintf(ctmp,"The %dth entry of channel vector requires channel %d, but the LSL stream \"%s\" has only %d channels.",k,channels[k],predicate.c_str(),streamchannels);
      throw TASCAR::ErrMsg(ctmp);
    }
  inlet = new lsl::stream_inlet(results[0]);
  srv = std::thread(&lslactor_t::service,this);
}

void lslactor_t::service()
{
  std::vector<double> sample;
  while( run_service ){
    double t(inlet->pull_sample( sample, 0.1 ));
    if( t != 0 ){
      if( channels[0] > -1 )
        transform.position.x = sample[channels[0]]*influence[0];
      if( channels[1] > -1 )
        transform.position.y = sample[channels[1]]*influence[1];
      if( channels[2] > -1 )
        transform.position.z = sample[channels[2]]*influence[2];
      if( channels[3] > -1 )
        transform.orientation.z = sample[channels[3]]*influence[3];
      if( channels[4] > -1 )
        transform.orientation.y = sample[channels[4]]*influence[4];
      if( channels[5] > -1 )
        transform.orientation.x = sample[channels[5]]*influence[5];
    }
  }
}

lslactor_t::~lslactor_t()
{
  run_service = false;
  srv.join();
  if( inlet )
    delete inlet;
}

void lslactor_t::update(uint32_t tp_frame,bool tp_rolling)
{
  if( incremental )
    add_transformation( transform, local );
  else
    set_transformation( transform, local );
}

REGISTER_MODULE(lslactor_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
