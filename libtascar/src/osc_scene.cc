#include "osc_scene.h"
#include "errorhandling.h"

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
  route_solo_p_t* h((route_solo_p_t*)user_data);
  if( h && (argc == 1) && (types[0]=='i') ){
    h->route->set_mute(argv[0]->i);
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

int osc_set_sound_gain_lin(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  sound_t* h((sound_t*)user_data);
  if( h && (argc == 1) && (types[0]=='f') ){
    h->set_gain_lin(argv[0]->f);
    return 0;
  }
  return 1;
}

int osc_set_diffuse_gain(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  diffuse_info_t* h((diffuse_info_t*)user_data);
  if( h && (argc == 1) && (types[0]=='f') ){
    h->set_gain_db(argv[0]->f);
    return 0;
  }
  return 1;
}

int osc_set_diffuse_gain_lin(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  diffuse_info_t* h((diffuse_info_t*)user_data);
  if( h && (argc == 1) && (types[0]=='f') ){
    h->set_gain_lin(argv[0]->f);
    return 0;
  }
  return 1;
}

int osc_set_receiver_gain(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  receivermod_object_t* h((receivermod_object_t*)user_data);
  if( h && (argc == 1) && (types[0]=='f') ){
    h->set_gain_db(argv[0]->f);
    return 0;
  }
  return 1;
}

int osc_set_receiver_lingain(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  receivermod_object_t* h((receivermod_object_t*)user_data);
  if( h && (argc == 1) && (types[0]=='f') ){
    h->set_gain_lin(argv[0]->f);
    return 0;
  }
  return 1;
}

int osc_set_receiver_fade(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  receivermod_object_t* h((receivermod_object_t*)user_data);
  if( h && (argc == 2) && (types[0]=='f') && (types[1]=='f') ){
    h->set_fade(argv[0]->f,argv[1]->f);
    return 0;
  }
  return 1;
}

void osc_scene_t::add_object_methods(TASCAR::osc_server_t* srv,TASCAR::Scene::object_t* o)
{
  srv->add_method("/"+scene->name+"/"+o->get_name()+"/pos","fff",osc_set_object_position,o);
  srv->add_method("/"+scene->name+"/"+o->get_name()+"/pos","ffffff",osc_set_object_position,o);
  srv->add_method("/"+scene->name+"/"+o->get_name()+"/zyxeuler","fff",osc_set_object_orientation,o);
}

void osc_scene_t::add_face_object_methods(TASCAR::osc_server_t* srv,TASCAR::Scene::face_object_t* o)
{
  srv->add_double("/"+scene->name+"/"+o->get_name()+"/reflectivity",&(o->reflectivity));
  srv->add_double("/"+scene->name+"/"+o->get_name()+"/damping",&(o->damping));
}

void osc_scene_t::add_face_group_methods(TASCAR::osc_server_t* srv,TASCAR::Scene::face_group_t* o)
{
  srv->add_double("/"+scene->name+"/"+o->get_name()+"/reflectivity",&(o->reflectivity));
  srv->add_double("/"+scene->name+"/"+o->get_name()+"/damping",&(o->damping));
}

void osc_scene_t::add_route_methods(TASCAR::osc_server_t* srv,TASCAR::Scene::route_t* o)
{
  route_solo_p_t* rs(new route_solo_p_t);
  rs->route = o;
  rs->anysolo = &(scene->anysolo);
  vprs.push_back(rs);
  srv->add_method("/"+scene->name+"/"+o->get_name()+"/mute","i",osc_route_mute,rs);
  srv->add_method("/"+scene->name+"/"+o->get_name()+"/solo","i",osc_route_solo,rs);
  srv->add_float("/"+scene->name+"/"+o->get_name()+"/targetlevel",&o->targetlevel);
}

void osc_scene_t::add_sound_methods(TASCAR::osc_server_t* srv,TASCAR::Scene::sound_t* s)
{
  std::string oldpref(srv->get_prefix());
  std::string ctlname("/"+scene->name+"/"+s->get_parent_name()+"/"+s->get_name());
  srv->set_prefix(ctlname);
  s->set_ctlname(ctlname);
  srv->add_method("/gain","f",osc_set_sound_gain,s);
  srv->add_method("/lingain","f",osc_set_sound_gain_lin,s);
  srv->add_float_db("/caliblevel",&(s->caliblevel));
  srv->add_uint("/ismmin",&(s->ismmin));
  srv->add_uint("/ismmax",&(s->ismmax));
  srv->add_uint("/layers",&(s->layers));
  srv->add_double("/size",&(s->size));
  s->plugins.add_variables( srv );
  srv->set_prefix(oldpref);
}

void osc_scene_t::add_diffuse_methods(TASCAR::osc_server_t* srv,TASCAR::Scene::diffuse_info_t* s)
{
  srv->add_method("/"+scene->name+"/"+s->object_t::get_name()+"/gain","f",osc_set_diffuse_gain,s);
  srv->add_method("/"+scene->name+"/"+s->object_t::get_name()+"/lingain","f",osc_set_diffuse_gain_lin,s);
  srv->add_float_db("/"+scene->name+"/"+s->object_t::get_name()+"/caliblevel",&(s->caliblevel));
  srv->add_uint("/"+scene->name+"/"+s->object_t::get_name()+"/layers",&(s->layers));
}

void osc_scene_t::add_receiver_methods(TASCAR::osc_server_t* srv,TASCAR::Scene::receivermod_object_t* s)
{
  std::string ctlname("/"+scene->name+"/"+s->object_t::get_name());
  s->set_ctlname(ctlname);
  std::string oldpref(srv->get_prefix());
  srv->set_prefix( ctlname );
  srv->add_method("/gain","f",osc_set_receiver_gain,s);
  srv->add_method("/lingain","f",osc_set_receiver_lingain,s);
  srv->add_double_db("/diffusegain",&(s->diffusegain));
  srv->add_method("/fade","ff",osc_set_receiver_fade,s);
  srv->add_uint("/ismmin",&(s->ismmin));
  srv->add_uint("/ismmax",&(s->ismmax));
  srv->add_uint("/layers",&(s->layers));
  srv->add_float_db("/caliblevel",&(s->caliblevel));
  s->add_variables( srv );
  srv->set_prefix( oldpref );
}

void osc_scene_t::add_child_methods(TASCAR::osc_server_t* srv)
{
  std::vector<object_t*> obj(scene->get_objects());
  for(std::vector<object_t*>::iterator it=obj.begin();it!=obj.end();++it){
    if( TASCAR::Scene::face_object_t* po=dynamic_cast<TASCAR::Scene::face_object_t*>(*it))
      add_face_object_methods(srv,po);
    if( TASCAR::Scene::face_group_t* po=dynamic_cast<TASCAR::Scene::face_group_t*>(*it))
      add_face_group_methods(srv,po);
    if( TASCAR::Scene::diffuse_info_t* po=dynamic_cast<TASCAR::Scene::diffuse_info_t*>(*it))
      add_diffuse_methods(srv,po);
    if( TASCAR::Scene::receivermod_object_t* po=dynamic_cast<TASCAR::Scene::receivermod_object_t*>(*it))
      add_receiver_methods(srv,po);
    add_object_methods(srv,*it);
    add_route_methods(srv,*it);
  }
  for(std::vector<sound_t*>::iterator it=scene->sounds.begin();it!=scene->sounds.end();++it){
    add_sound_methods(srv,*it);
  }
}

osc_scene_t::osc_scene_t(xmlpp::Element* xmlsrc, TASCAR::Scene::scene_t* scene_)
  : osc_server_t(xmlsrc->get_attribute_value("srv_addr"),xmlsrc->get_attribute_value("srv_port")),
    scene(scene_)
{
  if( !scene )
    throw TASCAR::ErrMsg("Invalid scene pointer");
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
