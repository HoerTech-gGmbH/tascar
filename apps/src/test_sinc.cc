#include "delayline.h"
#include <iostream>

using namespace TASCAR;

int main(int argc,char** argv)
{
  //sinctable_t sinc(5,64);
  //for(float x=-6;x<6;x+=0.1)
  //  std::cout << x << " " << sinc(x) << std::endl;
  varidelay_t d(100, 1, 1, 8, 1000);
  for(uint32_t k=0;k<20;k++)
    d.push(k==8);
  for(float x=0;x<20;x+=0.1){
    std::cout << x << " " << d.get_sinc(x) << std::endl;
  }
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
