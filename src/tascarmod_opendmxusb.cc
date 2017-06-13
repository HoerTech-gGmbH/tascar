#include "session.h"
#include "serviceclass.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h> 
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

class serialport_t {
public:
  serialport_t();
  ~serialport_t();
  int open( const char* dev, int speed, int parity, int stopbits );
  void set_interface_attribs( int speed, int parity, int stopbits );
  void set_blocking( int should_block );
  bool isopen();
  std::string readline(uint32_t maxlen,char delim);
  void close();
protected:
  int fd;
};
serialport_t::serialport_t()
  : fd(-1)
{
}

serialport_t::~serialport_t()
{
  if( fd > 0 )
    close();
}

bool serialport_t::isopen()
{
  return (fd > 0);
}

int serialport_t::open( const char* dev, int speed, int parity, int stopbits )
{
  fd = ::open( dev, O_RDWR | O_NOCTTY | O_SYNC );
  if( fd < 0 )
    throw TASCAR::ErrMsg(std::string("Unable to open device ")+dev);
  set_interface_attribs( speed, parity, stopbits );
  set_blocking( 1 );
  return fd;
}

void serialport_t::set_interface_attribs( int speed, int parity, int stopbits )
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr(fd, &tty) != 0)
    throw TASCAR::ErrMsg("Error from tcgetattr");
  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
  tty.c_iflag |= BRKINT;         // enable break processing
  tty.c_lflag = 0;                // no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  if( stopbits == 2 )
    tty.c_cflag |= CSTOPB;
  tty.c_cflag &= ~CRTSCTS;
  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    throw TASCAR::ErrMsg("error from tcsetattr");
  int flags; 
  ioctl(fd,TIOCMGET, &flags); 
  flags &= ~TIOCM_RTS; 
  flags &= ~(TIOCM_RTS|TIOCM_DTR);
  ioctl(fd,TIOCMSET, &flags); 
}

void serialport_t::set_blocking( int should_block )
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
    throw TASCAR::ErrMsg("error from tggetattr");
  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    throw TASCAR::ErrMsg( "error setting term attributes" );
}

void serialport_t::close()
{
  ::close(fd);
}

std::string serialport_t::readline(uint32_t maxlen,char delim)
{
  std::string r;
  while( isopen() && maxlen ){
    maxlen--;
    char c(0);
    if( ::read(fd,&c,1) == 1 ){
      if( c != delim )
        r+=c;
      else
        return r;
    }
  }
  return r;
}

class openDMXusb_t : public serialport_t {
public:
  openDMXusb_t(const char* device);
  void send(uint8_t universe, const std::vector<uint16_t>& data);
private:
  uint8_t msg[513];
  uint8_t* data_;
};

openDMXusb_t::openDMXusb_t(const char* device)
  : data_(&msg[1])
{
  msg[0] = 0;
  //B57600
  //B230400
  open(device,B230400,0,2);
}

void openDMXusb_t::send(uint8_t universe, const std::vector<uint16_t>& data)
{
  msg[0] = universe;
  msg[0] = 0;
  memset(data_,0,512);
  for(uint32_t k=0;k<data.size();++k)
    data_[k] = data[k];
  int result(0);
  result = ioctl(fd,TIOCSBRK,0); 
  usleep( 110 );
  result = ioctl(fd,TIOCCBRK,0);
  usleep( 25 );
  //tcsendbreak(fd,1);
  uint32_t n(write(fd,msg,513));
  //tcdrain(fd);
}

class artnetdmx_vars_t : public TASCAR::actor_module_t {
public:
  artnetdmx_vars_t( const TASCAR::module_cfg_t& cfg );
  virtual ~artnetdmx_vars_t();
protected:
  // config variables:
  std::string id;
  std::string device;
  double fps;
  uint32_t universe;
  TASCAR::spk_array_t fixtures;
  uint32_t channels;
  std::string parent;
  // derived params:
  TASCAR::named_object_t parentobj;
};

artnetdmx_vars_t::artnetdmx_vars_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg, false ), 
    fps(30),
    universe(0),
    fixtures(cfg.xmlsrc,"fixture"),
    channels(4),
    parentobj(NULL,"")
{
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(device);
  if( device.empty() )
    device = "/dev/ttyUSB0";
  GET_ATTRIBUTE(fps);
  GET_ATTRIBUTE(universe);
  GET_ATTRIBUTE(channels);
  GET_ATTRIBUTE(parent);
  std::vector<TASCAR::named_object_t> o(session->find_objects(parent));
  if( o.size()>0)
    parentobj = o[0];
}

artnetdmx_vars_t::~artnetdmx_vars_t()
{
}

class mod_artnetdmx_t : public artnetdmx_vars_t, public openDMXusb_t, public TASCAR::service_t {
public:
  mod_artnetdmx_t( const TASCAR::module_cfg_t& cfg );
  virtual ~mod_artnetdmx_t();
  virtual void update(uint32_t frame,bool running);
  virtual void service();
  static int osc_setval(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setval(uint32_t,double val);
  static int osc_setw(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setw(uint32_t,double val);
private:
  // internal variables:
  std::vector<uint16_t> dmxaddr;
  std::vector<uint16_t> dmxdata;
  std::vector<float> tmpdmxdata;
  std::vector<float> basedmx;
  std::vector<float> objval;
  std::vector<float> objw;
};

void mod_artnetdmx_t::setval(uint32_t k,double val)
{
  if( k<objval.size() )
    objval[k] = val;
}

int mod_artnetdmx_t::osc_setval(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  mod_artnetdmx_t* h((mod_artnetdmx_t*)user_data);
  for( int32_t k=0;k<argc;++k)
    if( types[k] == 'f' )
      h->setval(k,argv[k]->f);
  return 0;
}

void mod_artnetdmx_t::setw(uint32_t k,double val)
{
  if( k<objw.size() )
    objw[k] = val;
}

int mod_artnetdmx_t::osc_setw(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  mod_artnetdmx_t* h((mod_artnetdmx_t*)user_data);
  for( int32_t k=0;k<argc;++k)
    if( types[k] == 'f' )
      h->setw(k,argv[k]->f);
  return 0;
}

mod_artnetdmx_t::mod_artnetdmx_t( const TASCAR::module_cfg_t& cfg )
  : artnetdmx_vars_t( cfg ),
    openDMXusb_t(device.c_str())
{
  if( fixtures.size() == 0 )
    throw TASCAR::ErrMsg("No fixtures defined!");
  dmxaddr.resize(fixtures.size()*channels);
  dmxdata.resize(fixtures.size()*channels);
  basedmx.resize(fixtures.size()*channels);
  tmpdmxdata.resize(fixtures.size()*channels);
  for(uint32_t k=0;k<fixtures.size();++k){
    uint32_t startaddr(1);
    fixtures[k].get_attribute("addr",startaddr);
    std::vector<int32_t> lampdmx;
    fixtures[k].get_attribute("dmxval",lampdmx);
    lampdmx.resize(channels);
    for(uint32_t c=0;c<channels;++c){
      dmxaddr[channels*k+c] = (startaddr+c-1);
      basedmx[channels*k+c] = lampdmx[c];
    }
  }
  objval.resize(channels*obj.size());
  objw.resize(obj.size());
  for(uint32_t k=0;k<objw.size();++k)
    objw[k] = 1.0f;
  // register objval and objw as OSC variables:
  session->add_method("/"+id+"/dmx",std::string(objval.size(),'f').c_str(),&mod_artnetdmx_t::osc_setval,this);
  session->add_method("/"+id+"/w",std::string(objw.size(),'f').c_str(),&mod_artnetdmx_t::osc_setw,this);
  //
  start_service();
}

mod_artnetdmx_t::~mod_artnetdmx_t()
{
  stop_service();
}

void mod_artnetdmx_t::service()
{
  uint32_t waitusec(1000000/fps);
  std::vector<uint16_t> localdata;
  localdata.resize(512);
  usleep( 1000 );
  while( run_service ){
    for(uint32_t k=0;k<localdata.size();++k)
      localdata[k] = 0;
    for(uint32_t k=0;k<dmxaddr.size();++k)
      //localdata[k] = dmxaddr[k] + std::min((uint16_t)255,dmxdata[k]);
      localdata[dmxaddr[k]] = std::min((uint16_t)255,dmxdata[k]);
    send(universe,localdata);
    usleep( waitusec );
  }
  for(uint32_t k=0;k<localdata.size();++k)
    localdata[k] = 0;
  send(universe,localdata);
}

void mod_artnetdmx_t::update(uint32_t frame,bool running)
{
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    tmpdmxdata[k] = 0;
  if( parentobj.obj ){
    TASCAR::pos_t self_pos(parentobj.obj->get_location());
    TASCAR::zyx_euler_t self_rot(parentobj.obj->get_orientation());
    // do panning / copy data
    for(uint32_t kobj=0;kobj<obj.size();++kobj){
      TASCAR::pos_t pobj(obj[kobj].obj->get_location());
      pobj -= self_pos;
      pobj /= self_rot;
      //float dist(pobj.norm());
      pobj.normalize();
      for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
        // cos(az) = (pobj * pfixture)
        float caz(dot_prod(pobj,fixtures[kfix].unitvector));
        // 
        float w(powf(0.5f*(caz+1.0f),2.0f/objw[kobj]));
        for(uint32_t c=0;c<channels;++c)
          tmpdmxdata[channels*kfix+c] += w*objval[channels*kobj+c];
      }
    }
  }
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    dmxdata[k] = std::min(255.0f,std::max(0.0f,tmpdmxdata[k]+basedmx[k]));
}

REGISTER_MODULE(mod_artnetdmx_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

