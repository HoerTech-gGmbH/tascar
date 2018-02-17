#include "session.h"
#include <complex.h>

class mpu6050track_t : public TASCAR::actor_module_t {
public:
  mpu6050track_t( const TASCAR::module_cfg_t& cfg );
  ~mpu6050track_t();
  void prepare( chunk_cfg_t& );
  void update(uint32_t frame, bool running);
  void write_xml();
  static int osc_setrot(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_setrotypr(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_setrotgyr(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_calib(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setrot( const TASCAR::zyx_euler_t& e );
  void setrotypr( float* e );
  void setrotgyr( double t, double* gyr );
  void calib();
private:
  std::string id;
  std::string mode;
  uint32_t ypraxis;
  uint32_t gyraxis;
  double gyrscale;
  double scale;
  double tau;
  double _Complex z_mean;
  double c;
  double rot;
  bool b_calib;
  double tprev;
};

void mpu6050track_t::prepare( chunk_cfg_t& cf_ )
{
  actor_module_t::prepare( cf_ );
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

int mpu6050track_t::osc_setrotypr(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  // 0 = time
  // 1 = yaw
  // 2 = pitch
  // 3 = roll
  if( user_data && (argc==4) && (types[2]=='f') ){
    float data[3];
    data[0] = DEG2RAD*(argv[1]->f);
    data[1] = DEG2RAD*(argv[2]->f);
    data[2] = DEG2RAD*(argv[3]->f);
    ((mpu6050track_t*)user_data)->setrotypr( data );
  }
  return 0;
}

int mpu6050track_t::osc_setrotgyr(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  // 0 = time
  // 1 = yaw
  // 2 = pitch
  // 3 = roll
  if( user_data && (argc==7) && (types[0]=='d') ){
    double data[3];
    data[0] = DEG2RAD*(argv[4]->d);
    data[1] = DEG2RAD*(argv[5]->d);
    data[2] = DEG2RAD*(argv[6]->d);
    ((mpu6050track_t*)user_data)->setrotgyr( argv[0]->d, data );
  }
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

void mpu6050track_t::setrotypr( float* e )
{
  rot = e[std::min(2u,ypraxis)];
}

void mpu6050track_t::setrotgyr( double t, double* gyr )
{
  double g(gyr[std::min(2u,gyraxis)]);
  double dt(t-tprev);
  tprev = t;
  if( (0.0 < dt) && (dt < 0.1 ) ){
    rot += gyrscale*dt*g;
  }
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
    ypraxis(0),
    gyraxis(1),
    scale(1.0),
    gyrscale(0.06),
    tau(2.0),
    z_mean(1.0),
    c(0.999),
    rot(0),
    b_calib(false),
    tprev(0)
{
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(tau);
  GET_ATTRIBUTE(scale);
  GET_ATTRIBUTE(gyrscale);
  GET_ATTRIBUTE(ypraxis);
  GET_ATTRIBUTE(gyraxis);
  GET_ATTRIBUTE(mode);
  if( mode.empty() )
    mode = "euler";
  if( mode == "euler" )  
    session->add_method(id+"/euler","ffff",&mpu6050track_t::osc_setrot,this);
  else if( mode == "ypr" )
    session->add_method(id+"/ypr","ffff",&mpu6050track_t::osc_setrotypr,this);
  else if( mode == "gyr" )
    session->add_method(id+"/raw","diiiddd",&mpu6050track_t::osc_setrotgyr,this);
  else
    throw TASCAR::ErrMsg("Unknown mode \""+mode+"\".");
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
  double az(carg( z *conj( z_mean ) ));
  // check for normal values by using a comparison (result is false for nan and inf):
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
