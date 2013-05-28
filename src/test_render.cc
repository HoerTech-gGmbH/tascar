#include "acousticmodel.h"
#include "sinks.h"
#include "defs.h"
#include <iostream>

using namespace TASCAR;

void print_audio(const wave_t& a)
{
  for(unsigned int k=0;k<a.size();k++)
    std::cout << a[k] << std::endl;
}

int main(int argc,char** argv)
{
  uint32_t N(1024);
  pointsource_t src(N);
  src.audio[0] = 1;
  std::vector<pointsource_t*> sources(1,&src);
  //DEBUG(sources.size());
  reflector_t r1;
  reflector_t r2;
  std::vector<reflector_t*> reflectors;
  reflectors.push_back(&r1);
  reflectors.push_back(&r2);
  //DEBUG(reflectors.size());
  mirror_model_t mr(sources,reflectors,1);
  std::vector<pointsource_t*> mirrors(mr.get_mirrors());
  //DEBUG(mirrors.size());
  sources.insert(sources.end(),mirrors.begin(),mirrors.end());
  //DEBUG(sources.size());
  sink_omni_t sink(N);
  //DEBUG(N);
  std::vector<acoustic_model_t> world;
  for(uint32_t k=0;k<sources.size();k++){
    //DEBUG(k);
    acoustic_model_t am(44100,sources[k],&sink);
    //DEBUG(1);
    world.push_back(am);
    //DEBUG(world.size());
  }
  //DEBUG(world.size());
  for(uint32_t k=0;k<world.size();k++)
    world[k].process();
  print_audio(sink.audio);
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
