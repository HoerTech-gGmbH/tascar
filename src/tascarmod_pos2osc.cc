#include "session.h"

class pos2osc_t : public TASCAR::module_base_t {
public:
  pos2osc_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
  ~pos2osc_t();
  void write_xml();
  void update(uint32_t frame, bool running);
private:
  std::string url;
  std::string pattern;
  uint32_t ttl;
  lo_address target;
  std::vector<TASCAR::named_object_t> obj;
};

pos2osc_t::pos2osc_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
  : module_base_t(xmlsrc,session),
    ttl(1)
{
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(pattern);
  GET_ATTRIBUTE(ttl);
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
}

void pos2osc_t::write_xml()
{
  SET_ATTRIBUTE(url);
  SET_ATTRIBUTE(pattern);
  SET_ATTRIBUTE(ttl);
}

pos2osc_t::~pos2osc_t()
{
  lo_address_free(target);
}

//int pos2osc_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running)
void pos2osc_t::update(uint32_t tp_frame, bool tp_running)
{
  if( tp_running ){
    for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
      TASCAR::pos_t p;
      TASCAR::zyx_euler_t o;
      it->obj->get_6dof(p,o);
      std::string path;
      path = it->name + "/pos";
      lo_send(target,path.c_str(),"fff",p.x,p.y,p.z);
      path = it->name + "/rot";
      lo_send(target,path.c_str(),"fff",RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
    }
  }
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
