#ifndef IRRENDER_H
#define IRRENDER_H

#include "session_reader.h"
#include "audiochunks.h"
#include "render.h"

namespace TASCAR {

  class wav_render_t : public TASCAR::tsc_reader_t {
  public:
    wav_render_t(const std::string& tscname,const std::string& scene);
    void set_ism_order_range( uint32_t ism_min, uint32_t ism_max, bool b_0_14=false );
    void render(uint32_t fragsize,const std::string& ifname, const std::string& ofname,double starttime, bool b_dynamic);
    ~wav_render_t();
  protected:
    void add_scene(xmlpp::Element* e);
    std::string scene;
    render_core_t* pscene;
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
