#ifndef IRRENDER_H
#define IRRENDER_H

#include "session_reader.h"
#include "audiochunks.h"
#include "render.h"
#include <time.h>

namespace TASCAR {

  class wav_render_t : public TASCAR::tsc_reader_t {
  public:
    wav_render_t(const std::string& tscname,const std::string& scene, bool verbose=false);
    void set_ism_order_range( uint32_t ism_min, uint32_t ism_max );
    void render(uint32_t fragsize,const std::string& ifname, const std::string& ofname,double starttime, bool b_dynamic);
    void render_ir(uint32_t len,double fs, const std::string& ofname,double starttime);
    ~wav_render_t();
  protected:
    void add_scene(xmlpp::Element* e);
    std::string scene;
    render_core_t* pscene;
    bool verbose_;
  public:
    clock_t t0;
    clock_t t1;
    clock_t t2;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
