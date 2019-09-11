#include <iostream>
#include "session.h"
#include "errorhandling.h"
#include <unistd.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <errno.h>
#include "serviceclass.h"

class controller_t : public TASCAR::service_t {
public:
  controller_t();
  virtual ~controller_t();
  virtual void axis(uint32_t a, double value){};
  virtual void button(uint32_t b, bool value){};
  virtual void timeout() {};
};

controller_t::controller_t()
{
}

controller_t::~controller_t()
{
  stop_service();
}

class ctl_joystick_t : public controller_t {
public:
  ctl_joystick_t(const std::string& device);
  ~ctl_joystick_t();
  virtual void service();
private:
  int joy_fd;
};

#define NUMDEV 8
std::string devices[NUMDEV] = {
  "/dev/input/js0",
  "/dev/input/js1",
  "/dev/input/js2",
  "/dev/input/js3",
  "/dev/js0",
  "/dev/js1",
  "/dev/js2",
  "/dev/js3"
};

ctl_joystick_t::ctl_joystick_t(const std::string& device)
  : joy_fd(-1)
{
  if( device.empty() ){
  uint32_t dev(0);
  while( (dev < NUMDEV) && ((joy_fd = open(devices[dev].c_str(), O_RDONLY))==-1) ){
    dev++;
    if( dev >= NUMDEV ){
      TASCAR::add_warning("Warning: Unable to find a valid joystick device.");
      return;
    }
  }
  }else{
    joy_fd = open(device.c_str(), O_RDONLY);
    if( joy_fd == -1 )
      TASCAR::add_warning("Warning: Unable to find a valid joystick device at \""+device+"\".");
  }
}

ctl_joystick_t::~ctl_joystick_t()
{
  if( joy_fd != -1 )
    close( joy_fd );
}

void ctl_joystick_t::service()
{
  if( joy_fd == -1 )
    return;
  struct js_event js;
  while( run_service ){
    struct timeval timeo;
    timeo.tv_sec = 1;
    timeo.tv_usec = 0;
    fd_set readers;
    int r;
    FD_ZERO(&readers);
    FD_SET(joy_fd, &readers);
    r = select(joy_fd+1, &readers, NULL, NULL, &timeo);
    if (r < 0) {
      if ((errno == EAGAIN) || (errno == EINTR)){
	// do nothing.
      }else{
	throw TASCAR::ErrMsg("Error during select!");
      }
    }
    if (FD_ISSET(joy_fd, &readers)) {
      /* read the joystick state */
      if( sizeof(struct js_event) == read(joy_fd, &js, sizeof(struct js_event)) ){
	/* see what to do with the event */
	switch (js.type & ~JS_EVENT_INIT){
	case JS_EVENT_AXIS:
	  axis(js.number,js.value/32767.0);
	  break;
	case JS_EVENT_BUTTON:
	  button(js.number,js.value);
	  break;
	}
      }
    }else{
      timeout();
    }
  }
}

class axprop_t {
public:
  axprop_t(double& x,uint32_t ax_);
  void proc(uint32_t ax,double val);
  uint32_t ax;
  double threshold;
  double scale;
  double min;
  double max;
  double& x_;
};

axprop_t::axprop_t(double& x,uint32_t ax_)
  : ax(ax_),threshold(0.1),scale(1.0),min(-1000.0),max(1000.0),x_(x)
{
}

void axprop_t::proc(uint32_t ax_,double val)
{
  if( ax_ == ax ){
    if( fabs(val)>=threshold ){
      double lval(val);
      if( lval > 0 )
	lval -=threshold;
      else
	lval += threshold;
      double lscale(scale/(1.0-threshold));
      x_ = std::min(max,std::max(min,lscale*lval));
    }else
      x_ = 0;
  }
}

class joystick_var_t : public TASCAR::actor_module_t
{
public:
  joystick_var_t( const TASCAR::module_cfg_t& cfg );
protected:
  double d_tilt;
  TASCAR::pos_t velocity;
  TASCAR::zyx_euler_t rotvel;
  axprop_t x, y, r, tilt;
  double vscale;
  double maxnorm;
  bool dump_events;
  std::string url;
  std::string device;
};

joystick_var_t::joystick_var_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg, true ),
    d_tilt(0),
    x(velocity.x,1),
    y(velocity.y,0),
    r(rotvel.z,3),
    tilt(d_tilt,2),
    vscale(1.0),
    maxnorm(0),
    dump_events(false)
{
  tilt.scale = M_PI*0.5;
  tilt.threshold = 0;
  get_attribute_bool("dump_events",dump_events);
  GET_ATTRIBUTE(maxnorm);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(device);
  std::string preset;
  GET_ATTRIBUTE(preset);
  if( preset == "xbox360" ){
    x.ax=1;
    x.scale=-1;
    x.threshold=0.2;
    x.max=4;
    x.min=-4; 
    y.ax=0;
    y.scale=-1;
    y.threshold=0.2;
    y.max=4;
    y.min=-4; 
    r.ax=3;
    r.scale=-1;
    r.threshold=0.3;
    r.max=4;
    r.min=-4;
    tilt.ax=4;
    tilt.scale=-0.4;
    tilt.threshold=0.3;
    tilt.max=0.5*M_PI;
    tilt.min=-0.5*M_PI;
  }
  if( preset == "logitechX3d" ){
    x.ax=1;
    x.scale=-1;
    x.threshold=0.2;
    x.max=4;
    x.min=-4; 
    y.ax=0;
    y.scale=-1;
    y.threshold=0.2;
    y.max=4;
    y.min=-4; 
    r.ax=2;
    r.scale=-1;
    r.threshold=0.3;
    r.max=4;
    r.min=-4;
    tilt.ax=3;
    tilt.scale=-0.5*M_PI;
    tilt.threshold=0.3;
    tilt.max=0.5*M_PI;
    tilt.min=-0.5*M_PI;
  }
#define ATTRIB(x) get_attribute(#x "_ax",x.ax); get_attribute(#x "_scale",x.scale); get_attribute(#x "_threshold",x.threshold); get_attribute(#x "_max",x.max); get_attribute(#x "_min",x.min)
  ATTRIB(x);
  ATTRIB(y);
  ATTRIB(r);
  ATTRIB(tilt);
#undef ATTRIB
}

class joystick_t : public joystick_var_t, public ctl_joystick_t {
public:
  joystick_t( const TASCAR::module_cfg_t& cfg );
  virtual ~joystick_t();
  virtual void axis(uint32_t a, double value);
  virtual void button(uint32_t b, bool value);
  virtual void timeout() {};
  void prepare( chunk_cfg_t& );
  void update(uint32_t tp_frame,bool running);
private:
  lo_address lo_addr;
};

joystick_t::joystick_t( const TASCAR::module_cfg_t& cfg )
  : joystick_var_t( cfg ),
    ctl_joystick_t( device ),
    lo_addr(NULL)
{
  if( !url.empty() )
    lo_addr = lo_address_new_from_url(url.c_str());
  start_service();
}

void joystick_t::prepare( chunk_cfg_t& cf_ )
{
  actor_module_t::prepare(cf_);
  vscale = (double)n_fragment/(double)f_sample;
}

void joystick_t::update(uint32_t tp_frame,bool running)
{
  TASCAR::zyx_euler_t r(rotvel);
  r *= vscale;
  add_orientation(r);
  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
    it->obj->dorientation.y = d_tilt;
    TASCAR::pos_t v(velocity);
    v *= vscale;
    v *= TASCAR::zyx_euler_t(it->obj->dorientation.z,0,0);
    it->obj->dlocation += v;
    if( maxnorm > 0 ){
      double dnorm(it->obj->dlocation.norm());
      if( dnorm > maxnorm )
	it->obj->dlocation *= maxnorm/dnorm;
    }
  }
}

joystick_t::~joystick_t()
{
  if( lo_addr )
    lo_address_free(lo_addr);
}

void joystick_t::axis(uint32_t a, double value)
{
  if( lo_addr )
    lo_send( lo_addr, "/axis", "if", a, value );
  if( dump_events )
    std::cout << "axis " << a << "  val " << value << std::endl;
  x.proc(a,value);
  y.proc(a,value);
  r.proc(a,value);
  tilt.proc(a,value);
}

void joystick_t::button(uint32_t a, bool value)
{
  if( lo_addr )
    lo_send( lo_addr, "/button", "ii", a, value );
  if( dump_events )
    std::cout << "button " << a << "  val " << value << std::endl;
}

REGISTER_MODULE(joystick_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

