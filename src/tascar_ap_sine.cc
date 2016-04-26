#include "audioplugin.h"

class sine_t : public TASCAR::audioplugin_base_t {
public:
  sine_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
  void process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos);
private:
  double f;
  double a;
  double t;
};

sine_t::sine_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name,parentname),
    f(1000),
    a(0.001),
    t(0)
{
  GET_ATTRIBUTE(f);
  GET_ATTRIBUTE_DB(a);
  DEBUG(a);
}

void sine_t::process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos)
{
  for(uint32_t k=0;k<chunk.n;++k){
    chunk.d[k] += a*sin(PI2*f*t);
    t+=t_sample;
  }
}

REGISTER_AUDIOPLUGIN(sine_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
