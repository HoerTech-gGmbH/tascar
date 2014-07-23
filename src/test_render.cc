#include "xmlconfig.h"
#include "defs.h"
#include <iostream>

int main(int argc,char** argv)
{
  std::string s("abc${TEST1}/${TEST2}xy");
  DEBUG(s);
  DEBUG(TASCAR::env_expand(s));
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
