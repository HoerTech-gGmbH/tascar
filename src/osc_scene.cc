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

int osc_route_mute(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::Scene::object_t* h((TASCAR::Scene::object_t*)user_data);
  if( h && (argc == 1) && (types[0]=='i') ){
    h->set_mute(argv[0]->i);
    return 0;
  }
  return 1;
}

int osc_route_solo(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  route_solo_p_t* h((route_solo_p_t*)user_data);
  if( h && (argc == 1) && (types[0]=='i') ){
    h->route->set_solo(argv[0]->i,*(h->anysolo));
    return 0;
  }
  return 1;
}

int osc_set_sound_gain(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  sound_t* h((sound_t*)user_data);
  if( h && (argc == 1) && (types[0]=='f') ){
    h->set_gain_db(argv[0]->f);
    return 0;
  }
  return 1;
}

int osc_set_diffuse_gain(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  src_diffuse_t* h((src_diffuse_t*)user_data);
  if( h && (argc == 1) && (types[0]=='f') ){
    h->set_gain_db(argv[0]->f);
    return 0;
  }
  return 1;
}

void osc_scene_t::add_object_methods(TASCAR::Scene::object_t* o)
{
  add_method("/"+name+"/"+o->get_name()+"/pos","fff",osc_set_object_position,o);
  add_method("/"+name+"/"+o->get_name()+"/pos","ffffff",osc_set_object_position,o);
  add_method("/"+name+"/"+o->get_name()+"/zyxeuler","fff",osc_set_object_orientation,o);
}


void osc_scene_t::add_route_methods(TASCAR::Scene::route_t* o)
{
  add_method("/"+name+"/"+o->get_name()+"/mute","i",osc_route_mute,o);
  route_solo_p_t* rs(new route_solo_p_t);
  rs->route = o;
  rs->anysolo = &anysolo;
  vprs.push_back(rs);
  add_method("/"+name+"/"+o->get_name()+"/solo","i",osc_route_solo,rs);
}


void osc_scene_t::add_sound_methods(TASCAR::Scene::sound_t* s)
{
  add_method("/"+name+"/"+s->get_parent_name()+"/"+s->get_name()+"/gain","f",osc_set_sound_gain,s);
}

void osc_scene_t::add_diffuse_methods(TASCAR::Scene::src_diffuse_t* s)
{
  add_method("/"+name+"/"+s->object_t::get_name()+"/gain","f",osc_set_diffuse_gain,s);
}

void osc_scene_t::add_child_methods()
{
  std::vector<object_t*> obj(get_objects());
  for(std::vector<object_t*>::iterator it=obj.begin();it!=obj.end();++it){
    add_object_methods(*it);
    add_route_methods(*it);
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    add_diffuse_methods(&(*it));
  }
  std::vector<sound_t*> sounds(linearize_sounds());
  for(std::vector<sound_t*>::iterator it=sounds.begin();it!=sounds.end();++it){
    add_sound_methods(*it);
  }
}

osc_scene_t::osc_scene_t(const std::string& srv_addr, const std::string& srv_port, xmlpp::Element* xmlsrc)
  : scene_t(xmlsrc),
    osc_server_t(srv_addr,srv_port)
{
}

osc_scene_t::~osc_scene_t()
{
  for(std::vector<route_solo_p_t*>::iterator it=vprs.begin();it!=vprs.end();++it)
    delete *it;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
