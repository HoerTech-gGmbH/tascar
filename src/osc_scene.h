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
      void add_child_methods(TASCAR::osc_server_t*);
    private:
      void add_object_methods(TASCAR::osc_server_t*,TASCAR::Scene::object_t* o);
      void add_route_methods(TASCAR::osc_server_t*,TASCAR::Scene::route_t* r);
      void add_sound_methods(TASCAR::osc_server_t*,TASCAR::Scene::sound_t* s);
      void add_diffuse_methods(TASCAR::osc_server_t*,TASCAR::Scene::src_diffuse_t* s);
      void add_face_object_methods(TASCAR::osc_server_t*,TASCAR::Scene::face_object_t* s);
      void add_face_group_methods(TASCAR::osc_server_t*,TASCAR::Scene::face_group_t* s);
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
