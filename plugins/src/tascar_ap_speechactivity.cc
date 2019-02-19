#include "audioplugin.h"
#include <lo/lo.h>
#include <lsl_cpp.h>
#include "errorhandling.h"

class speechactivity_t : public TASCAR::audioplugin_base_t {
public:
  speechactivity_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void prepare( chunk_cfg_t& cf_ );
  void release();
  ~speechactivity_t();
private:
  lo_address lo_addr;
  lsl::stream_outlet* lsl_outlet;
  double tauenv;
  double tauonset;
  double threshold;
  std::string url;
  std::string path;
  std::vector<double> intensity;
  std::vector<int32_t> active;
  std::vector<int32_t> prevactive;
  std::vector<double> dactive;
  std::vector<int32_t> onset;
};

speechactivity_t::speechactivity_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    tauenv(1),
    tauonset(1),
    threshold(0.0056234),
    url("osc.udp://localhost:9999/"),
    path( "/"+get_fullname() )
{
  GET_ATTRIBUTE(tauenv);
  GET_ATTRIBUTE(tauonset);
  GET_ATTRIBUTE_DBSPL(threshold);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(path);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
}

void speechactivity_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
  lo_addr = lo_address_new_from_url(url.c_str());
  lsl_outlet = new lsl::stream_outlet(lsl::stream_info(get_fullname(),"level",cf_.n_channels,cf_.f_fragment,lsl::cf_int32));
  intensity = std::vector<double>( cf_.n_channels, 0.0 );
  active = std::vector<int32_t>( cf_.n_channels, 0 );
  prevactive = std::vector<int32_t>( cf_.n_channels, 0 );
  dactive = std::vector<double>( cf_.n_channels, 0 );
  onset = std::vector<int32_t>( cf_.n_channels, 0 );
}

void speechactivity_t::release()
{
  delete lsl_outlet;
  lo_address_free(lo_addr);
}

speechactivity_t::~speechactivity_t()
{
}

void speechactivity_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  TASCAR_ASSERT(chunk.size()==intensity.size());
  double  lpc1(exp(-PI2/(tauenv*f_fragment)));
  double lpc2(pow(2.0,-1.0/(tauonset*f_fragment)));
  float v2threshold(threshold*threshold);
  for(uint32_t ch=0;ch<chunk.size();++ch){
    // first get signal intensity:
    intensity[ch] = (1.0-lpc1)*chunk[ch].ms() + lpc1*intensity[ch];
    // speech activity is given if the intensity is above the given
    // threshold:
    active[ch] = (intensity[ch]>v2threshold);
    // calculate low-pass filtered temporal derivative of activity, to
    // get the onsets:
    dactive[ch] = (active[ch]-prevactive[ch]) + lpc2*dactive[ch];
    prevactive[ch] = active[ch];
    onset[ch] = (dactive[ch] > 0.5)*active[ch] + active[ch];
    char ctmp[1024];
    sprintf(ctmp,"%s%d",path.c_str(),ch);
    lo_send( lo_addr, ctmp, "i", onset[ch] );
  }
  lsl_outlet->push_sample( onset );
}

REGISTER_AUDIOPLUGIN(speechactivity_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
