#include "session.h"

class msg_t : public TASCAR::xml_element_t {
public:
  msg_t( xmlpp::Element* );
  ~msg_t();
  std::string path;
  lo_message msg;
private:
  msg_t( const msg_t& );
};

msg_t::msg_t( xmlpp::Element* e )
  : TASCAR::xml_element_t(e),
    msg(lo_message_new())
{
  GET_ATTRIBUTE(path);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      TASCAR::xml_element_t tsne(sne);
      if( sne->get_name() == "f" ){
        double v(0);
        tsne.get_attribute("v",v);
        lo_message_add_float(msg,v);
      }
      if( sne->get_name() == "i" ){
        int32_t v(0);
        tsne.get_attribute("v",v);
        lo_message_add_int32(msg,v);
      }
      if( sne->get_name() == "s" ){
        std::string v("");
        tsne.get_attribute("v",v);
        lo_message_add_string(msg,v.c_str());
      }
    }
  }
}

msg_t::~msg_t()
{
  lo_message_free(msg);
}

class nearsensor_t : public TASCAR::module_base_t {
public:
  nearsensor_t( const TASCAR::module_cfg_t& cfg );
  ~nearsensor_t();
  void write_xml();
  void update(uint32_t frame, bool running);
private:
  std::string url;
  std::string pattern;
  std::string parent;
  double radius;
  uint32_t mode;
  uint32_t ttl;
  std::string path;
  lo_address target;
  std::vector<TASCAR::named_object_t> obj;
  TASCAR::named_object_t parentobj;
  //lo_message own_msg;
  std::vector<msg_t*> vmsg1;
  std::vector<msg_t*> vmsg2;
  uint32_t hitcnt_state;
};

nearsensor_t::nearsensor_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    radius(1.0),
    mode(0),
    ttl(1),
    parentobj(NULL,""),
    //own_msg(lo_message_new()),
    hitcnt_state(0)
{
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(ttl);
  GET_ATTRIBUTE(pattern);
  GET_ATTRIBUTE(parent);
  GET_ATTRIBUTE(radius);
  GET_ATTRIBUTE(mode);
  GET_ATTRIBUTE(path);
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
  std::vector<TASCAR::named_object_t> o(session->find_objects(parent));
  if( o.size()>0)
    parentobj = o[0];
  vmsg1.push_back(new msg_t(e));
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "msgapp" )
        vmsg1.push_back(new msg_t(sne));
      if( sne->get_name() == "msgdep" )
        vmsg2.push_back(new msg_t(sne));
    }
  }
}

void nearsensor_t::write_xml()
{
  SET_ATTRIBUTE(url);
  SET_ATTRIBUTE(ttl);
  SET_ATTRIBUTE(pattern);
  SET_ATTRIBUTE(parent);
  SET_ATTRIBUTE(radius);
  SET_ATTRIBUTE(mode);
  SET_ATTRIBUTE(path);
}

nearsensor_t::~nearsensor_t()
{
  for( std::vector<msg_t*>::iterator it=vmsg1.begin();it!=vmsg1.end();++it)
    delete *it;
  for( std::vector<msg_t*>::iterator it=vmsg2.begin();it!=vmsg2.end();++it)
    delete *it;
  lo_address_free(target);
}

void nearsensor_t::update(uint32_t tp_frame, bool tp_rolling)
{
  uint32_t hitcnt(0);
  if( parentobj.obj ){
    TASCAR::pos_t self_pos(parentobj.obj->get_location());
    for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
      switch( mode ){
      case 0:
        {
          TASCAR::pos_t p(it->obj->get_location());
          if( distance(self_pos,p) < radius )
            ++hitcnt;
        }
        break;
      case 1:
        {
          TASCAR::Scene::src_object_t* srco(dynamic_cast<TASCAR::Scene::src_object_t*>(it->obj));
          if( srco ){
            for( std::vector<TASCAR::Scene::sound_t*>::iterator sit=srco->sound.begin();sit!=srco->sound.end();++sit){
              if( distance(self_pos,(*sit)->position) < radius )
                ++hitcnt;
            }
          }
        }
        break;
      }
    }
  }
  if( hitcnt > hitcnt_state )
    for( std::vector<msg_t*>::iterator it=vmsg1.begin();it!=vmsg1.end();++it)
      if( !(*it)->path.empty() )
        lo_send_message( target, (*it)->path.c_str(), (*it)->msg );
  if( hitcnt < hitcnt_state )
    for( std::vector<msg_t*>::iterator it=vmsg2.begin();it!=vmsg2.end();++it)
      if( !(*it)->path.empty() )
        lo_send_message( target, (*it)->path.c_str(), (*it)->msg );
  hitcnt_state = hitcnt;
}

REGISTER_MODULE(nearsensor_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
