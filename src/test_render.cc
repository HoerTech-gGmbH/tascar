#include "acousticmodel.h"
#include "sinks.h"

using namespace TASCAR;

int main(int argc,char** argv)
{
  pointsource_t src(4);
  sink_omni_t sink(4);
  acoustic_model_t acmod(44100,src,sink);
  acmod.process();
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
