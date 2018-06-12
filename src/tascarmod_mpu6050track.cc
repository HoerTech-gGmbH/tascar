#include "session.h"
#include <complex.h>

class mpu6050track_t : public TASCAR::actor_module_t {
public:
  mpu6050track_t( const TASCAR::module_cfg_t& cfg );
  ~mpu6050track_t();
  void prepare( chunk_cfg_t& );
  void update(uint32_t frame, bool running);
  static int osc_setrot(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_setrotypr(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_setrotgyr(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_calib(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_calib1(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setrot( float* e );
  void setrotypr( float* e );
  void setrotgyr( double t, double* gyr, double* acc );
  void calib();
  void calib1( bool b_cal );
private:
  std::string id;
  std::string mode;
  std::string calib_start;
  std::string calib_end;
  uint32_t ypraxis;
  uint32_t gyraxis;
  uint32_t zaxis;
  uint32_t rotaxis;
  double rotscale;
  double zscale;
  double gyrscale;
  double scale;
  double tau;
  double tauz;
  double _Complex z_mean;
  double c;
  double cz;
  double rot;
  double zshift;
  double zshiftlp;
  bool b_calib;
  double tprev;
  lo_message msg_calib;
};

void mpu6050track_t::prepare( chunk_cfg_t& cf_ )
{
  actor_module_t::prepare( cf_ );
  c = exp(-1.0/(tau*f_fragment));
  cz = exp(-1.0/(tauz*f_fragment));
}

int mpu6050track_t::osc_calib(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data )
    ((mpu6050track_t*)user_data)->calib();
  return 0;
}

int mpu6050track_t::osc_calib1(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data )
    ((mpu6050track_t*)user_data)->calib1(argv[0]->i);
  return 0;
}

int mpu6050track_t::osc_setrot(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  // 0 = time
  // 1 = rx
  // 2 = ry <- here up-axis
  // 3 = rz
  if( user_data && (argc==4) && (types[2]=='f') ){
    float data[3];
    data[0] = argv[3]->f;
    data[1] = argv[2]->f;
    data[2] = argv[1]->f;
    ((mpu6050track_t*)user_data)->setrot( data );
  }
  return 1;
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
    double acc[3];
    data[0] = DEG2RAD*(argv[4]->d);
    data[1] = DEG2RAD*(argv[5]->d);
    data[2] = DEG2RAD*(argv[6]->d);
    acc[0] = argv[1]->i;
    acc[1] = argv[2]->i;
    acc[2] = argv[3]->i;
    ((mpu6050track_t*)user_data)->setrotgyr( argv[0]->d, data, acc );
  }
  return 0;
}

void mpu6050track_t::setrot( float* data )
{
  TASCAR::zyx_euler_t e(rotscale*data[rotaxis],0,0);
  TASCAR::pos_t x(0,1,0);
  x *= e;
  double az(x.azim());
  if( (-4 < az) && (az < 4) )
    rot = az;
}

void mpu6050track_t::setrotypr( float* e )
{
  rot = e[std::min(2u,ypraxis)];
}

void mpu6050track_t::setrotgyr( double t, double* gyr, double* acc )
{
  double g(gyr[std::min(2u,gyraxis)]);
  double dt(t-tprev);
  tprev = t;
  if( (0.0 < dt) && (dt < 0.1 ) ){
    rot += gyrscale*dt*g;
  }
  if( (0 <= zaxis) && (zaxis < 3) ){
    zshift = zscale*acc[zaxis]*0.00006103515625;
  }
}

void mpu6050track_t::calib()
{
  b_calib = true;
}

void mpu6050track_t::calib1( bool bcal )
{
  b_calib = true;
  if( bcal ){
    if( !calib_start.empty() )
      session->dispatch_data_message(calib_start.c_str(),msg_calib);
  }else{
    if( !calib_end.empty() )
      session->dispatch_data_message(calib_end.c_str(),msg_calib);
  }
}

mpu6050track_t::mpu6050track_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg ),
    ypraxis(0),
    gyraxis(1),
    zaxis(-1),
  rotaxis(1),
  rotscale(DEG2RAD),
    zscale(1),
    gyrscale(0.06),
    scale(1.0),
    tau(2.0),
    tauz(0.5),
    z_mean(1.0),
    c(0.999),
    rot(0),
    zshift(0),
    zshiftlp(0),
    b_calib(false),
    tprev(0),
    msg_calib(lo_message_new())
{
  lo_message_add_int32(msg_calib,1);
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(tau);
  GET_ATTRIBUTE(tauz);
  GET_ATTRIBUTE(scale);
  GET_ATTRIBUTE(gyrscale);
  GET_ATTRIBUTE(rotaxis);
  if( (rotaxis < 0) || (2 < rotaxis) )
    throw TASCAR::ErrMsg("Invalid rotation axis (must be 0, 1 or 2).");
  GET_ATTRIBUTE(rotscale);
  GET_ATTRIBUTE(ypraxis);
  GET_ATTRIBUTE(gyraxis);
  GET_ATTRIBUTE(zaxis);
  GET_ATTRIBUTE(zscale);
  GET_ATTRIBUTE(calib_start);
  GET_ATTRIBUTE(calib_end);
  GET_ATTRIBUTE(mode);
  if( mode.empty() )
    mode = "euler";
  if( mode == "euler" )  
    session->add_method(id+"/euler","ffff",&mpu6050track_t::osc_setrot,this);
  else if( mode == "ypr" )
    session->add_method(id+"/ypr","ffff",&mpu6050track_t::osc_setrotypr,this);
  else if( mode == "gyr" )
    session->add_method(id+"/raw","diiiddd",&mpu6050track_t::osc_setrotgyr,this);
  else if( mode == "rotnew" )  
    session->add_method("/rot","ffff",&mpu6050track_t::osc_setrot,this);
  else
    throw TASCAR::ErrMsg("Unknown mode \""+mode+"\".");
  session->add_method(id+"/calib","fff",&mpu6050track_t::osc_calib,this);
  session->add_method(id+"/calibration","i",&mpu6050track_t::osc_calib1,this);
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
  // z-shifting:
  zshiftlp = cz*zshiftlp + (1.0-cz)*zshift;
  set_location( TASCAR::pos_t(0, 0, zshiftlp ) );
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
