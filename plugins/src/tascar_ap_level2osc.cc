#include "audioplugin.h"
#include "errorhandling.h"
#include <lo/lo.h>

class level2osc_t : public TASCAR::audioplugin_base_t {
public:
  level2osc_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void prepare( chunk_cfg_t& cf_ );
  void release();
  ~level2osc_t();
private:
  bool sendwhilestopped;
  uint32_t skip;
  std::string url;
  std::string path;
  // derived variables:
  lo_address lo_addr;
  uint32_t skipcnt;
  lo_message msg;
  lo_arg** oscmsgargv;
};

level2osc_t::level2osc_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    sendwhilestopped(false),
    skip(0),
    url("osc.udp://localhost:9999/"),
    path("/level"),
    skipcnt(0)
{
  GET_ATTRIBUTE_BOOL(sendwhilestopped);
  GET_ATTRIBUTE(skip);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(path);
  lo_addr = lo_address_new_from_url(url.c_str());
}

void level2osc_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
  msg = lo_message_new();
  // time:
  lo_message_add_float(msg,0);
  // levels:
  for( uint32_t k=0;k<n_channels;++k)
    lo_message_add_float(msg,0);
  oscmsgargv = lo_message_get_argv(msg);
}

void level2osc_t::release()
{
  lo_message_free( msg );
  audioplugin_base_t::release();
}

level2osc_t::~level2osc_t()
{
  lo_address_free(lo_addr);
}

void level2osc_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  if( chunk.size() != n_channels )
    throw TASCAR::ErrMsg("Programming error (invalid channel number, expected "+TASCAR::to_string(n_channels)+", got "+TASCAR::to_string(chunk.size())+").");
  if( tp.rolling || sendwhilestopped ){
    if( skipcnt ){
      skipcnt--;
    }else{
      // pack data:
      oscmsgargv[0]->f = tp.object_time_seconds;
      for( uint32_t ch=0;ch<n_channels;++ch)
        oscmsgargv[ch+1]->f = chunk[ch].spldb();
      // send message:
      lo_send_message( lo_addr, path.c_str(), msg );
      skipcnt = skip;
    }
  }
}

REGISTER_AUDIOPLUGIN(level2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
