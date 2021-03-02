#include "session.h"

class motionpath_t : public TASCAR::actor_module_t {
public:
  motionpath_t( const TASCAR::module_cfg_t& cfg );
  ~motionpath_t();
  void update(uint32_t frame, bool running);
  static int osc_go(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void go(double start,double end);
private:
  double time;
  double stoptime;
  bool running;
  std::string id;
  bool active;
  bool tascartime;
  TASCAR::track_t location;
  TASCAR::euler_track_t orientation;
  double sampledorientation;
};

int motionpath_t::osc_go(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc==2) && (types[0]=='f') && (types[1]=='f') )
    ((motionpath_t*)user_data)->go(argv[0]->f,argv[1]->f);
  return 0;
}

void motionpath_t::go(double start,double end)
{
  running = false;
  stoptime = end;
  time = start;
  running = true;
}


motionpath_t::motionpath_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg ),
    time(0),
    stoptime(3153600000),// 100 years from now
    running(false),
    active(true),
    tascartime(false),
    sampledorientation(0)
{
  GET_ATTRIBUTE_BOOL_(active);
  GET_ATTRIBUTE_BOOL_(tascartime);
  GET_ATTRIBUTE_(id);
  GET_ATTRIBUTE_(sampledorientation);
  if( id.empty() )
    id = "motionpath";
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "position")){
      //xml_location = sne;
      location.read_xml(sne);
    }
    if( sne && ( sne->get_name() == "orientation")){
      //xml_orientation = sne;
      orientation.read_xml(sne);
    }
    if( sne && (sne->get_name() == "creator")){
      xmlpp::Node::NodeList subnodes = sne->get_children();
      location.edit(subnodes);
      TASCAR::track_t::iterator it_old=location.end();
      double old_azim(0);
      double new_azim(0);
      for(TASCAR::track_t::iterator it=location.begin();it!=location.end();++it){
        if( it_old != location.end() ){
          TASCAR::pos_t p=it->second;
          p -= it_old->second;
          new_azim = p.azim();
          while( new_azim - old_azim > M_PI )
            new_azim -= 2*M_PI;
          while( new_azim - old_azim < -M_PI )
            new_azim += 2*M_PI;
          orientation[it_old->first] = TASCAR::zyx_euler_t(new_azim,0,0);
          old_azim = new_azim;
        }
        if( TASCAR::distance(it->second,it_old->second) > 0 )
          it_old = it;
      }
    }
  }
  if( !tascartime ){
    session->add_method("/"+id+"/go","ff",&motionpath_t::osc_go,this);
    session->add_bool_true("/"+id+"/start",&running);
    session->add_bool_false("/"+id+"/stop",&running);
    session->add_double("/"+id+"/locate",&time);
    session->add_double("/"+id+"/stoptime",&stoptime);
  }
  session->add_bool("/"+id+"/active",&active);
}

motionpath_t::~motionpath_t()
{
}

void motionpath_t::update(uint32_t tp_frame,bool tp_rolling)
{
  if( !active )
    return;
  if( time > stoptime ){
    time = stoptime;
    running = false;
  }
  if( running )
    time += t_fragment;
  double ltime(time);
  if( tascartime )
    ltime = tp_frame*t_sample;

  TASCAR::c6dof_t c6dof_;
  c6dof_.position = location.interp(ltime);
  if( sampledorientation == 0 )
    c6dof_.orientation = orientation.interp(ltime);
  else{
    double tp(location.get_time(location.get_dist(ltime)-sampledorientation));
    TASCAR::pos_t pdt(c6dof_.position);
    pdt -= location.interp(tp);
    if( sampledorientation < 0 )
      pdt *= -1.0;
    c6dof_.orientation.z = pdt.azim();
    c6dof_.orientation.y = pdt.elev();
    c6dof_.orientation.x = 0.0;
  }
  //
  //
  //TASCAR::zyx_euler_t dr(orientation.interp(ltime));
  //TASCAR::pos_t dp(location.interp(ltime));
  for(auto iobj : obj ){
    iobj.obj->dorientation = c6dof_.orientation;
    iobj.obj->dlocation = c6dof_.position;
  }
}

REGISTER_MODULE(motionpath_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
