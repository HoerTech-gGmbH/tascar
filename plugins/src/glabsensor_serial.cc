#include "glabsensorplugin.h"
#include "serviceclass.h"
#include <fcntl.h> 
#include <termios.h>
#include "errorhandling.h"
#include "serialport.h"
#include <lsl_cpp.h>

using namespace TASCAR;

class gls_serial_t : public sensorplugin_base_t, public service_t {
public:
  gls_serial_t( const sensorplugin_cfg_t& cfg );
  virtual ~gls_serial_t() throw();
private:
  virtual void service();
  std::string device;
  uint32_t baudrate;
  uint32_t charsize;
  uint32_t channels;
  double offset;
  double scale;
  serialport_t* dev;
  uint32_t timeoutcnt;
  uint32_t prevtimeout;
  double v0;
  double c1, c2;
  uint32_t initcnt;
  lsl::stream_outlet* outlet;
  int baud;
  std::vector<float> x;
};

gls_serial_t::gls_serial_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_base_t(cfg),
    device("/dev/ttyS0"),
    baudrate(38400),
    charsize(8),
    channels(1),
    offset(0.0),
    scale(1.0),
    dev(NULL),
    timeoutcnt(0),
    baud(B9600)
{
  GET_ATTRIBUTE(device);
  if( device.empty() )
    device = "/dev/rfcomm1";
  GET_ATTRIBUTE(baudrate);
  GET_ATTRIBUTE(charsize);
  GET_ATTRIBUTE(offset);
  GET_ATTRIBUTE(scale);
  GET_ATTRIBUTE(channels);
  switch( baudrate ){
  case 50 : baud = B50; break;
  case 75 : baud = B75; break;
  case 110 : baud = B110; break;
  case 134 : baud = B134; break;
  case 150 : baud = B150; break;
  case 200 : baud = B200; break;
  case 300 : baud = B300; break;
  case 600 : baud = B600; break;
  case 1200 : baud = B1200; break;
  case 1800 : baud = B1800; break;
  case 2400 : baud = B2400; break;
  case 4800 : baud = B4800; break;
  case 9600 : baud = B9600; break;
  case 19200 : baud = B19200; break;
  case 38400 : baud = B38400; break;
  case 57600 : baud = B57600; break;
  case 115200 : baud = B115200; break;
  case 230400 : baud = B230400; break;
  default: throw ErrMsg("Invalid baud rate.");
  }
  x.resize(channels);
  lsl::stream_info streaminfo(get_name(),modname,channels,lsl::IRREGULAR_RATE,lsl::cf_double64);
  outlet = new lsl::stream_outlet(streaminfo);
  dev = new serialport_t();
  start_service();
}

gls_serial_t::~gls_serial_t() throw()
{
  stop_service();
  delete dev;
  delete outlet;
}

void gls_serial_t::service()
{
  bool b_open(false);
  while( (!b_open) && run_service ){
    try{
      dev->open(device.c_str(),baud,0);
      b_open = true;
    }
    catch( const std::exception& e ){
      add_critical( e.what() );
      sleep(1);
    }
  }
  while( b_open && run_service ){
    if( dev && dev->isopen() ){
      std::stringstream resp(dev->readline(1000,'\n'));
      for( uint32_t ch=0;ch<channels;++ch){
        x[ch] = std::numeric_limits<float>::infinity();
        resp >> x[ch];
        x[ch] = (x[ch] - offset)*scale;
      }
      timeoutcnt = 0;
      outlet->push_sample(x);
      alive();
    }
  }
  if( b_open )
    dev->close();
}

REGISTER_SENSORPLUGIN(gls_serial_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
