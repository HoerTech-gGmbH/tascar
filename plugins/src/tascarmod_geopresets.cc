#include "session.h"
#include <complex>
#include <mutex>

class geopresets_t : public TASCAR::actor_module_t {
public:
  geopresets_t( const TASCAR::module_cfg_t& cfg );
  ~geopresets_t();
  void update(uint32_t frame, bool );
  static int osc_setpreset(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setpreset( const std::string& );
private:
  double time;
  double duration;
  double current_duration;
  bool running;
  std::string id;
  bool enable;
  std::string preset;
  std::map<std::string,TASCAR::pos_t> positions;
  std::map<std::string,TASCAR::zyx_euler_t> orientations;
  TASCAR::zyx_euler_t current_r;
  TASCAR::pos_t current_p;
  TASCAR::zyx_euler_t new_r;
  TASCAR::pos_t new_p;
  TASCAR::zyx_euler_t old_r;
  TASCAR::pos_t old_p;
  std::complex<float> phase;
  std::complex<float> d_phase;
  std::mutex mtx;
  bool b_newpos;
};

int geopresets_t::osc_setpreset(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc==1) && (types[0]=='s') )
    ((geopresets_t*)user_data)->setpreset(&(argv[0]->s));
  return 0;
}

void geopresets_t::setpreset( const std::string& s )
{
  mtx.lock();
  preset = s;
  b_newpos = true;
  mtx.unlock();
}


geopresets_t::geopresets_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg ),
    time(0),
    duration(2.0),
    current_duration(duration),
    running(false),
    enable(true),
    phase(1.0f),
    d_phase(1.0f),
    b_newpos(false)
{
  GET_ATTRIBUTE(duration);
  GET_ATTRIBUTE_BOOL(enable);
  GET_ATTRIBUTE(id);
  if( id.empty() )
    id = "geopresets";
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "preset")){
      xml_element_t pres(sne);
      std::string name;
      pres.get_attribute("name",name);
      if( pres.has_attribute("position") ){
        TASCAR::pos_t pos;
        pres.get_attribute("position",pos);
        positions[name] = pos;
      }
      if( pres.has_attribute("orientation") ){
        TASCAR::pos_t pos;
        pres.get_attribute("orientation",pos);
	pos *= DEG2RAD;
        orientations[name] = TASCAR::zyx_euler_t(pos.x,pos.y,pos.z);
      }
    }
  }
  session->add_method("/"+id,"s",&geopresets_t::osc_setpreset,this);
  session->add_double("/"+id+"/duration",&duration);
  session->add_bool("/"+id+"/enable",&enable);
}

geopresets_t::~geopresets_t()
{
}

void geopresets_t::update(uint32_t tp_frame,bool tp_rolling)
{
  if( !enable )
    return;
  if( b_newpos ){
    if( mtx.try_lock() ){
      current_duration = duration;
      old_p = current_p;
      old_r = current_r;
      auto pos_it(positions.find(preset));
      if( pos_it != positions.end() )
        new_p = pos_it->second;
      auto rot_it(orientations.find(preset));
      if( rot_it != orientations.end() )
        new_r = rot_it->second;
      const std::complex<double> i(0, 1);
      d_phase = std::exp( i*M_PI*t_fragment/duration );
      phase = 1.0f;
      b_newpos = false;
      running = true;
      time = 0.0f;
      mtx.unlock();
    }
  }
  if( time > current_duration ){
    time = current_duration;
    phase = -1.0f;
    running = false;
  }
  if( running ){
    time += t_fragment;
    phase *= d_phase;
  }
  // raised-cosine weights:
  float w(0.5-0.5*std::real(phase));
  float w1(1.0f-w);
  // get weighted position:
  current_p.x = w*new_p.x + w1*old_p.x;
  current_p.y = w*new_p.y + w1*old_p.y;
  current_p.z = w*new_p.z + w1*old_p.z;
  // get weighted orientation:
  current_r.x = w*new_r.x + w1*old_r.x;
  current_r.y = w*new_r.y + w1*old_r.y;
  current_r.z = w*new_r.z + w1*old_r.z;
  // apply transformation:
  for(std::vector<TASCAR::named_object_t>::iterator iobj=obj.begin();iobj!=obj.end();++iobj){
    iobj->obj->dorientation = current_r;
    iobj->obj->dlocation = current_p;
  }
}

REGISTER_MODULE(geopresets_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
