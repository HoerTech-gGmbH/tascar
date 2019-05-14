#include "session.h"

class pos2osc_t : public TASCAR::module_base_t {
public:
  pos2osc_t( const TASCAR::module_cfg_t& cfg );
  ~pos2osc_t();
  void update(uint32_t frame, bool running);
private:
  std::string url;
  std::string pattern;
  uint32_t mode;
  uint32_t ttl;
  bool transport;
  std::string avatar;
  double lookatlen;
  bool triggered;
  bool ignoreorientation;
  bool trigger;
  lo_address target;
  std::vector<TASCAR::named_object_t> obj;
};

pos2osc_t::pos2osc_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    mode(0),
    ttl(1),
    transport(true),
    lookatlen(1.0),
    triggered(false),
    ignoreorientation(false),
    trigger(true)
{
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(pattern);
  GET_ATTRIBUTE(ttl);
  GET_ATTRIBUTE(mode);
  GET_ATTRIBUTE_BOOL(transport);
  GET_ATTRIBUTE(avatar);
  GET_ATTRIBUTE(lookatlen);
  GET_ATTRIBUTE_BOOL(triggered);
  GET_ATTRIBUTE_BOOL(ignoreorientation);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  if( pattern.empty() )
    pattern = "/*/*";
  target = lo_address_new_from_url(url.c_str());
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+url+"\".");
  lo_address_set_ttl(target,ttl);
  obj = session->find_objects(pattern);
  if( !obj.size() )
    throw TASCAR::ErrMsg("No target objects found (target pattern: \""+pattern+"\").");
  if( mode == 4 ){
    cfg.session->add_bool_true("/pos2osc/"+avatar+"/trigger",&trigger);
    cfg.session->add_bool("/pos2osc/"+avatar+"/active",&trigger);
    cfg.session->add_bool("/pos2osc/"+avatar+"/triggered",&triggered);
    cfg.session->add_double("/pos2osc/"+avatar+"/lookatlen",&lookatlen);
  }
  if( triggered )
    trigger = false;

}

pos2osc_t::~pos2osc_t()
{
  lo_address_free(target);
}

//int pos2osc_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_rolling)
void pos2osc_t::update(uint32_t tp_frame, bool tp_rolling)
{
  if( trigger && ((!triggered && (tp_rolling || (!transport))) || triggered) ){
    for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
      // copy position from parent object:
      const TASCAR::pos_t& p(it->obj->c6dof.position);
      TASCAR::zyx_euler_t o(it->obj->c6dof.orientation);
      if( ignoreorientation )
        o = it->obj->c6dof_nodelta.orientation;
      std::string path;
      switch( mode ){
      case 0 :
        path = it->name + "/pos";
        lo_send(target,path.c_str(),"fff",p.x,p.y,p.z);
        path = it->name + "/rot";
        lo_send(target,path.c_str(),"fff",RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
        break;
      case 1:
        path = it->name + "/pos";
        lo_send(target,path.c_str(),"ffffff",p.x,p.y,p.z,RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
        break;
      case 2:
        path = "/tascarpos";
        lo_send(target,path.c_str(),"sffffff",it->name.c_str(),p.x,p.y,p.z,RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
        break;
      case 3:
        path = "/tascarpos";
        lo_send(target,path.c_str(),"sffffff",it->obj->get_name().c_str(),p.x,p.y,p.z,RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
        break;
      case 4:
        path = "/"+avatar;
        if( lookatlen > 0 )
          lo_send(target,path.c_str(),"sffff","/lookAt",p.x,p.y,p.z,lookatlen);
        else
          lo_send(target,path.c_str(),"sfff","/lookAt",p.x,p.y,p.z);
        break;
      }
    }
  }
  if( triggered )
    trigger = false;
}

REGISTER_MODULE(pos2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
