#include "audioplugin.h"
#include <lo/lo.h>

class onsetdetector_t : public TASCAR::audioplugin_base_t {
public:
  onsetdetector_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
  void ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void prepare(double srate,uint32_t fragsize);
  void release();
  ~onsetdetector_t();
private:
  lo_address lo_addr;
  double tau;
  double taumin;
  double fadelen;
  double threshold;
  std::string url;
  std::string path;
  std::string side;
  const std::string side_l;
  const std::string side_r;
  bool b_autoside;
  double lpc1;
  double lpc11;
  double env;
  double ons;
  int8_t crit;
  int8_t crit1;
  int8_t detect;
  double time_since_last;
};

onsetdetector_t::onsetdetector_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name,parentname),
    tau(1),
    taumin(0.05),
    fadelen(1),
    threshold(0.01),
    url("osc.udp://localhost:9999/"),
    side_l("L"),
    side_r("R"),
    b_autoside(false),
    lpc1(0.0),
    lpc11(1.0),
    env(0.0),
    ons(0.0),
    crit(0),crit1(0),detect(0),
    time_since_last(0)
{
  GET_ATTRIBUTE(tau);
  GET_ATTRIBUTE(taumin);
  GET_ATTRIBUTE(fadelen);
  GET_ATTRIBUTE(side);
  GET_ATTRIBUTE_DBSPL(threshold);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(path);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  lo_addr = lo_address_new_from_url(url.c_str());
}

void onsetdetector_t::prepare(double srate,uint32_t fragsize)
{
  lpc1 = exp(-1.0/(tau*f_sample));
  lpc11 = 1.0-lpc1;
}

void onsetdetector_t::release()
{
}

onsetdetector_t::~onsetdetector_t()
{
  lo_address_free(lo_addr);
}

void onsetdetector_t::ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  const char* this_side(side.c_str());
  uint32_t N(chunk.size());
  float v2threshold(threshold*threshold);
  for(uint32_t k=0;k<N;++k){
    time_since_last += t_sample;
    float v2(chunk[k]);
    v2 *= v2;
    env = lpc1*env + lpc11*std::max(v2,v2threshold);
    if( v2 > ons )
      ons = v2;
    else
      ons = lpc1*ons + lpc11*v2;
    crit1 = crit;
    crit = (ons > env);
    detect = (crit > crit1) && (time_since_last > taumin);
    if( detect ){
      if( side.empty() ){
        if( b_autoside ){
          this_side = side_l.c_str();
          b_autoside = false;
        }else{
          this_side = side_r.c_str();
          b_autoside = true;
        }
      }
      lo_send( lo_addr, path.c_str(), "ssffff", "/hitAt", this_side, pos.x, pos.y, pos.z, sqrt(ons) );
      time_since_last = 0;
    }
  }
}

REGISTER_AUDIOPLUGIN(onsetdetector_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
