#include "session.h"

class dummy_t : public TASCAR::actor_module_t {
public:
  dummy_t( const TASCAR::module_cfg_t& cfg );
  ~dummy_t();
  void write_xml();
  void update(uint32_t frame, bool running);
  void configure(double srate,uint32_t fragsize);
private:
};

dummy_t::dummy_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg )
{
  DEBUG(1);
  DEBUG(f_sample);
  DEBUG(f_fragment);
  DEBUG(t_sample);
  DEBUG(t_fragment);
  DEBUG(n_fragment);
}

void dummy_t::configure( double srate, uint32_t fragsize )
{
  DEBUG(1);
  DEBUG(srate);
  DEBUG(fragsize);
  DEBUG(f_sample);
  DEBUG(f_fragment);
  DEBUG(t_sample);
  DEBUG(t_fragment);
  DEBUG(n_fragment);
}

void dummy_t::write_xml()
{
  DEBUG(1);
}

dummy_t::~dummy_t()
{
  DEBUG(1);
}

void dummy_t::update(uint32_t frame,bool running)
{
  DEBUG(1);
}

REGISTER_MODULE(dummy_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
