#include "audiostates.h"
#include "errorhandling.h"
#include "defs.h"

chunk_cfg_t::chunk_cfg_t( double f_sample_, uint32_t n_fragment_, uint32_t n_channels_ )
  :  f_sample(f_sample_),
     n_fragment(n_fragment_),
     n_channels(n_channels_)
{
  update();
}

void chunk_cfg_t::update()
{
  f_fragment = f_sample/n_fragment;
  t_sample = 1.0/std::max(EPS,f_sample);
  t_fragment = 1.0/std::max(EPS,f_fragment);
  t_inc = 1.0/std::max(EPS,(double)n_fragment);
}

audiostates_t::audiostates_t()
  : is_prepared_(false),
    preparecount(0)
{
}

void audiostates_t::prepare( chunk_cfg_t& cf_ )
{
  preparecount++;
#ifdef TSCDEBUG
  DEBUG(this);
  if( is_prepared_ )
    throw TASCAR::ErrMsg("Prepare called before release");
#endif
  is_prepared_ = true;
  *(chunk_cfg_t*)this = cf_;
  update();
}

void audiostates_t::release( )
{
#ifdef TSCDEBUG
  DEBUG(this);
  if( !is_prepared_ ){
    DEBUG(preparecount);
    throw TASCAR::ErrMsg("Release called without prepare");
  }
#endif
  is_prepared_ = false;
}

audiostates_t::~audiostates_t()
{
#ifdef TSCDEBUG
  DEBUG(this);
  if( is_prepared_ )
    throw TASCAR::ErrMsg("still in prepared state at end.");
#endif
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

