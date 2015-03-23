#include "session.h"

class pendulum_t : public TASCAR::module_base_t {
public:
  pendulum_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
  ~pendulum_t();
  void write_xml();
  void update(double t, bool running);
private:
  double amplitude;
  double frequency;
  double decaytime;
  double starttime;
  TASCAR::pos_t distance;
};

pendulum_t::pendulum_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
  : actor_module_t(xmlsrc,session),
    amplitude(45),
    frequency(0.5),
    decaytime(40),
    starttime(10),
    distance(0,0,-2)
{
  GET_ATTRIBUTE(amplitude);
  GET_ATTRIBUTE(frequency);
  GET_ATTRIBUTE(decaytime);
  GET_ATTRIBUTE(starttime);
  GET_ATTRIBUTE(distance);
}

void pendulum_t::write_xml()
{
  SET_ATTRIBUTE(amplitude);
  SET_ATTRIBUTE(frequency);
  SET_ATTRIBUTE(decaytime);
  SET_ATTRIBUTE(starttime);
  SET_ATTRIBUTE(distance);
}

pendulum_t::~pendulum_t()
{
}

void pendulum_t::update(double time,bool running)
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
  return 0;
}

REGISTER_MODULE(pendulum_t);
REGISTER_MODULE_UPDATE(pendulum_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
