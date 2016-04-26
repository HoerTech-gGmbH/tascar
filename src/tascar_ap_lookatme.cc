#include "audioplugin.h"
#include <lo/lo.h>
#include <fftw3.h>

class lookatme_t : public TASCAR::audioplugin_base_t {
public:
  lookatme_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
  void process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos);
  void prepare(double srate,uint32_t fragsize);
  void release();
  ~lookatme_t();
private:
  lo_address lo_addr;
  double tau;
  double fadelen;
  double threshold;
  std::string animation;
  std::string url;
  std::vector<std::string> paths;
  std::string self_;
  double lpc1;
  double rms;
  bool waslooking;
};

lookatme_t::lookatme_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name,parentname),
    tau(1),
    fadelen(1),
    threshold(0.01),
    url("osc.udp://localhost:9999/"),
    self_(parentname),
    lpc1(0.0),
    rms(0.0),
    waslooking(false)
{
  GET_ATTRIBUTE(tau);
  GET_ATTRIBUTE(fadelen);
  GET_ATTRIBUTE_DBSPL(threshold);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(paths);
  GET_ATTRIBUTE(animation);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  lo_addr = lo_address_new_from_url(url.c_str());
}

void lookatme_t::prepare(double srate,uint32_t fragsize)
{
  lpc1 = exp(-1.0/(tau*f_fragment));
  rms = 0;
  waslooking = false;
}

void lookatme_t::release()
{
}

lookatme_t::~lookatme_t()
{
  lo_address_free(lo_addr);
}

void lookatme_t::process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos)
{
  rms = lpc1*rms + (1.0-lpc1)*chunk.rms();
  if(rms > threshold ){
    if(!waslooking ){
      // send lookatme values to osc target:
      for(std::vector<std::string>::iterator s=paths.begin();s!=paths.end();++s)
        lo_send( lo_addr, s->c_str(), "sffff", "/lookAt", pos.x, pos.y, pos.z, fadelen );
      if( !animation.empty() )
        lo_send( lo_addr, self_.c_str(), "ss", "/animation", animation.c_str() );
      waslooking = true;
    }
  }else{
    // below threshold, release:
    waslooking = false;
  }
}

REGISTER_AUDIOPLUGIN(lookatme_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
