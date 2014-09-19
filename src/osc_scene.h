#ifndef OSC_SCENE_H
#define OSC_SCENE_H

#include "scene.h"
#include "osc_helper.h"

namespace TASCAR {

  class route_solo_p_t {
  public:
    TASCAR::Scene::route_t* route;
    uint32_t* anysolo;
  };

  namespace Scene {

    class osc_scene_t : public TASCAR::Scene::scene_t, public TASCAR::osc_server_t {
    public:
      osc_scene_t(xmlpp::Element* xmlsrc);
      ~osc_scene_t();
    protected:
      void add_child_methods();
    private:
      void add_object_methods(TASCAR::Scene::object_t* o);
      void add_route_methods(TASCAR::Scene::route_t* r);
      void add_sound_methods(TASCAR::Scene::sound_t* s);
      void add_diffuse_methods(TASCAR::Scene::src_diffuse_t* s);
      std::vector<route_solo_p_t*> vprs;
    };

  }

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
