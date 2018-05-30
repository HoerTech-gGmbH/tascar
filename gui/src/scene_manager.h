#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "session.h"

class scene_manager_t 
{
public:
  scene_manager_t();
  virtual ~scene_manager_t();
  void scene_load(const std::string& fname);
  void scene_destroy();
  void scene_new();
protected:
  pthread_mutex_t mtx_scene;
  TASCAR::session_t* session;
};

std::string load_any_file(const std::string& fname);

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
