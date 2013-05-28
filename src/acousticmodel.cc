#include "acousticmodel.h"

using namespace TASCAR;

pointsource_t::pointsource_t(uint32_t chunksize)
  : audio(chunksize)
{
}

acoustic_model_t::acoustic_model_t(double fs,pointsource_t& src,sink_t& sink,const std::vector<obstacle_t>& obstacles)
  : src_(src),sink_(sink),obstacles_(obstacles),
    audio(src.audio.size()),
    chunksize(audio.size()),
    dt(1.0/chunksize),
    delayline(480000,fs,340.0)
{
}
 
void acoustic_model_t::process()
{
  pos_t prel(sink_.relative_position(src_.position));
  double nextdistance(prel.norm());
  double ddist((nextdistance-distance)*dt);
  for(uint32_t k=0;k<chunksize;k++){
    distance+=ddist;
    delayline.push(src_.audio[k]);
    audio[k] = delayline.get_dist(distance)/std::max(0.1,distance);
  }
  sink_.add_source(prel,audio);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
