#ifndef OSC_SCENE_H
#define OSC_SCENE_H

#include "scene.h"
#include "osc_helper.h"

class osc_scene_t : public TASCAR::Scene::scene_t, public TASCAR::osc_server_t {
public:
  osc_scene_t(const std::string& srv_addr, const std::string& srv_port, const std::string& cfg_file);
protected:
  void add_object_methods();
private:
  void add_object_methods(TASCAR::Scene::object_t* o);
};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
