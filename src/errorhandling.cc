#include "defs.h"
#include "errorhandling.h"

TASCAR::ErrMsg::ErrMsg(const std::string& msg) 
  : std::string(msg) 
{
#ifdef TSCDEBUG
  std::cerr << "-Error: " << msg << std::endl;
#endif
}

TASCAR::ErrMsg::~ErrMsg() throw()
{
}

const char* TASCAR::ErrMsg::what() const throw ()
{
  return c_str(); 
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
