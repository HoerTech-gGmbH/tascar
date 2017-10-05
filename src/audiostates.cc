#include "audiostates.h"
#include "errorhandling.h"
#include "defs.h"

audiostates_t::audiostates_t()
  : f_sample(1),
    f_fragment(1),
    t_sample(1),
    t_fragment(1),
    t_inc(1),
    n_fragment(1),
    is_prepared_(false),
    preparecount(0)
{
}

void audiostates_t::prepare( double fs, uint32_t fragsize )
{
  preparecount++;
#ifdef TSCDEBUG
  DEBUG(this);
  if( is_prepared_ )
    throw TASCAR::ErrMsg("Prepare called before release");
#endif
  is_prepared_ = true;
  f_sample = fs;
  f_fragment = fs/fragsize;
  t_sample = 1.0/std::max(EPS,f_sample);
  t_fragment = 1.0/std::max(EPS,f_fragment);
  t_inc = 1.0/std::max(EPS,(double)fragsize);
  n_fragment = fragsize;
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

