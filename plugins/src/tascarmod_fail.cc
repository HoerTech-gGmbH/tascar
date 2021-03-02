#include "session.h"

class fail_t : public TASCAR::module_base_t {
public:
  fail_t( const TASCAR::module_cfg_t& cfg );
  ~fail_t();
  void update(uint32_t frame, bool running);
  void configure();
  void release();
private:
  bool failinit;
  bool failprepare;
  bool failrelease;
  bool failupdate;
};

fail_t::fail_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    failinit(false),
    failprepare(false),
    failrelease(false),
    failupdate(false)
{
  GET_ATTRIBUTE_BOOL_(failinit);
  GET_ATTRIBUTE_BOOL_(failprepare);
  GET_ATTRIBUTE_BOOL_(failrelease);
  GET_ATTRIBUTE_BOOL_(failupdate);
  if( failinit )
    throw TASCAR::ErrMsg("init.");
}

void fail_t::release()
{
  if( failrelease )
    throw TASCAR::ErrMsg("release.");
  module_base_t::release();
}

void fail_t::configure()
{
  DEBUG(this);
  if( failprepare )
    throw TASCAR::ErrMsg("prepare.");
  module_base_t::configure( );
}

fail_t::~fail_t()
{
}

void fail_t::update(uint32_t frame,bool running)
{
  if( failupdate )
    throw TASCAR::ErrMsg("update.");
}

REGISTER_MODULE(fail_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
