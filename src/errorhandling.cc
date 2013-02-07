#include "defs.h"
#include "errorhandling.h"
#include <iostream>

TASCAR::ErrMsg::ErrMsg(const std::string& msg) 
  : std::string(msg) 
{
//  DEBUG(msg);
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
