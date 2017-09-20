#include "session.h"
#include "jackclient.h"

#define SQRT12 0.70710678118654757274f

class route_vars_t : public TASCAR::module_base_t {
public:
  route_vars_t( const TASCAR::module_cfg_t& cfg );
  ~route_vars_t();
protected:
  std::string id;
  uint32_t channels;
  double gain;
};

class route_t : public route_vars_t, public jackc_t {
public:
  route_t( const TASCAR::module_cfg_t& cfg );
  ~route_t();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
private:
};

route_vars_t::route_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    channels(1),
    gain(1.0)
{
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(channels);
  GET_ATTRIBUTE_DB(gain);
  get_attribute("lingain",gain);
}

route_t::route_t( const TASCAR::module_cfg_t& cfg )
  : route_vars_t( cfg ),
    jackc_t(id)
{
  session->add_double_db("/"+id+"/gain",&gain);
  session->add_double("/"+id+"/lingain",&gain);
  for(uint32_t k=0;k<channels;++k){
    char ctmp[1024];
    sprintf(ctmp,"in.%d",k);
    add_input_port(ctmp);
    sprintf(ctmp,"out.%d",k);
    add_output_port(ctmp);
  }
  activate();
}

route_vars_t::~route_vars_t()
{
}

route_t::~route_t()
{
  deactivate();
}

int route_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  for(uint32_t ch=0;ch<std::min(sIn.size(),sOut.size());++ch){
    for(uint32_t k=0;k<n;++k){
      sOut[ch][k] = gain*sIn[ch][k];
    }
  }
  return 0;
}

REGISTER_MODULE(route_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

