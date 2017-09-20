#include "filterclass.h"
#include <iostream>

using namespace TASCAR;

int main(int argc,char** argv)
{
  fsplit_t fsnotch( 100, fsplit_t::notch, 3);
  fsplit_t fssine( 100, fsplit_t::sine, 3);
  fsplit_t fstria( 100, fsplit_t::tria, 3);
  fsplit_t fstriald( 100, fsplit_t::triald, 3);
  for(uint32_t k=0;k<30;k++){
    fsnotch.push(k==0);
    fssine.push(k==0);
    fstria.push(k==0);
    fstriald.push(k==0);
    float v1;
    float v2;
    fsnotch.get( v1, v2 );
    std::cout << " " << v1 << " " << v2;
    fssine.get( v1, v2 );
    std::cout << " " << v1 << " " << v2;
    fstria.get( v1, v2 );
    std::cout << " " << v1 << " " << v2;
    fstriald.get( v1, v2 );
    std::cout << " " << v1 << " " << v2;
    std::cout << "\n";
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
