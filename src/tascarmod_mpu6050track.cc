#include "session.h"
#include <complex.h>

class mpu6050track_t : public TASCAR::actor_module_t {
public:
  mpu6050track_t( const TASCAR::module_cfg_t& cfg );
  ~mpu6050track_t();
  void prepare(double srate,uint32_t fragsize);
  void update(uint32_t frame, bool running);
  void write_xml();
  static int osc_setrot(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_calib(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setrot( const TASCAR::zyx_euler_t& e );
  void calib();
private:
  std::string id;
  double scale;
  double tau;
  double _Complex z_mean;
  double c;
  double rot;
  bool b_calib;
};

void mpu6050track_t::prepare(double srate, uint32_t fragsize)
{
  actor_module_t::prepare( srate, fragsize );
  c = exp(-1.0/(tau*f_fragment));
}

int mpu6050track_t::osc_calib(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data )
    ((mpu6050track_t*)user_data)->calib();
  return 0;
}

int mpu6050track_t::osc_setrot(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  // 0 = time
  // 1 = rx
  // 2 = ry <- here up-axis
  // 3 = rz
  if( user_data && (argc==4) && (types[2]=='f') )
    ((mpu6050track_t*)user_data)->setrot( TASCAR::zyx_euler_t(DEG2RAD*(argv[3]->f), DEG2RAD*(argv[2]->f), DEG2RAD*(argv[1]->f)) );
  return 0;
}

void mpu6050track_t::setrot( const TASCAR::zyx_euler_t& e )
{
  TASCAR::pos_t x(0,1,0);
  x *= e;
  double tmp(x.z);
  x.z = x.y;
  x.y = tmp;
  double az(x.azim());
  if( (-4 < az) && (az < 4) )
    rot = az;
}

void mpu6050track_t::calib()
{
  b_calib = true;
}

void mpu6050track_t::write_xml()
{
  actor_module_t::write_xml();
  SET_ATTRIBUTE(id);
  SET_ATTRIBUTE(tau);
  SET_ATTRIBUTE(scale);
}

mpu6050track_t::mpu6050track_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg ),
    scale(1.0),
    tau(2.0),
    z_mean(1.0),
    c(0.999),
    rot(0),
    b_calib(false)
{
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(tau);
  GET_ATTRIBUTE(scale);
  session->add_method(id+"/euler","ffff",&mpu6050track_t::osc_setrot,this);
  session->add_method(id+"/calib","fff",&mpu6050track_t::osc_calib,this);
}

mpu6050track_t::~mpu6050track_t()
{
}

void mpu6050track_t::update(uint32_t tp_frame,bool tp_rolling)
{
  double _Complex z(cexp(I*rot));
  if( b_calib ){
    b_calib = false;
    z_mean = z;
  }
  z_mean = c*z_mean + (1.0-c)*z;
  double az(carg( z / z_mean ));
  if( (-4 < az) && (az < 4) )
    set_orientation( TASCAR::zyx_euler_t( scale*az, 0, 0 ) );
  else
    set_orientation( TASCAR::zyx_euler_t( 0, 0, 0 ) );
}

REGISTER_MODULE(mpu6050track_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
