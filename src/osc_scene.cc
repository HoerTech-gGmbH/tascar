#include "osc_scene.h"
#include "coordinates.h"

using namespace TASCAR;
using namespace TASCAR::Scene;

int osc_set_object_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::Scene::object_t* h((TASCAR::Scene::object_t*)user_data);
  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
    pos_t r;
    r.x = argv[0]->f;
    r.y = argv[1]->f;
    r.z = argv[2]->f;
    h->dlocation = r;
    return 0;
  }
  if( h && (argc == 6) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f')
      && (types[3]=='f') && (types[4]=='f') && (types[5]=='f') ){
    pos_t rp;
    rp.x = argv[0]->f;
    rp.y = argv[1]->f;
    rp.z = argv[2]->f;
    h->dlocation = rp;
    zyx_euler_t ro;
    ro.z = DEG2RAD*argv[3]->f;
    ro.y = DEG2RAD*argv[4]->f;
    ro.x = DEG2RAD*argv[5]->f;
    h->dorientation = ro;
    return 0;
  }
  return 1;
}

int osc_set_object_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::Scene::object_t* h((TASCAR::Scene::object_t*)user_data);
  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
    zyx_euler_t r;
    r.z = DEG2RAD*argv[0]->f;
    r.y = DEG2RAD*argv[1]->f;
    r.x = DEG2RAD*argv[2]->f;
    h->dorientation = r;
    return 0;
  }
  if( h && (argc == 1) && (types[0]=='f') ){
    zyx_euler_t r;
    r.z = DEG2RAD*argv[0]->f;
    h->dorientation = r;
    return 0;
  }
  return 1;
}

void osc_scene_t::add_object_methods(TASCAR::Scene::object_t* o)
{
  add_method("/"+o->get_name()+"/pos","fff",osc_set_object_position,o);
  add_method("/"+o->get_name()+"/pos","ffffff",osc_set_object_position,o);
  add_method("/"+o->get_name()+"/zyxeuler","fff",osc_set_object_orientation,o);
}

void osc_scene_t::add_object_methods()
{
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    add_object_methods(&(*it));
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    add_object_methods(&(*it));
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
    add_object_methods(&(*it));
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    add_object_methods(&(*it));
}

osc_scene_t::osc_scene_t(const std::string& srv_addr, const std::string& srv_port, const std::string& cfg_file)
  : scene_t(cfg_file),
    osc_server_t(srv_addr,srv_port)
{
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
