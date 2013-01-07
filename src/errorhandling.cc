#include "errorhandling.h"

TASCAR::ErrMsg::ErrMsg(const std::string& msg) 
  : std::string(msg) 
{
}

TASCAR::ErrMsg::~ErrMsg() throw()
{
}

const char* TASCAR::ErrMsg::what() 
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
