/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "glabsensorplugin.h"
#include "serviceclass.h"
#include <fcntl.h> 
#include <termios.h>
#include "errorhandling.h"
#include <lsl_cpp.h>

using namespace TASCAR;

class serialport_t {
public:
  serialport_t();
  ~serialport_t();
  int open( const char* dev, int speed, int parity );
  void set_interface_attribs( int speed, int parity );
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

int serialport_t::open( const char* dev, int speed, int parity )
{
  fd = ::open( dev, O_RDWR | O_NOCTTY | O_SYNC );
  if( fd < 0 )
    throw ErrMsg(std::string("Unable to open device \"")+dev+"\".");
  set_interface_attribs( speed, parity );
  set_blocking( 1 );
  return fd;
}

void serialport_t::set_interface_attribs( int speed, int parity )
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr(fd, &tty) != 0)
    throw ErrMsg("Error from tcgetattr");
  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
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
  tty.c_cflag &= ~CRTSCTS;
  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    throw ErrMsg("error from tcsetattr");
}

void serialport_t::set_blocking( int should_block )
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
    throw ErrMsg("error from tggetattr");
  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    throw ErrMsg( "error setting term attributes" );
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

class eog_t : public sensorplugin_drawing_t, public service_t {
public:
  eog_t( const sensorplugin_cfg_t& cfg );
  virtual ~eog_t() throw();
  virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
private:
  virtual void service();
  std::string device;
  uint32_t baudrate;
  uint32_t charsize;
  double offset;
  double scale;
  //double range;
  std::string unit;
  serialport_t* dev;
  uint32_t timeoutcnt;
  uint32_t prevtimeout;
  double v0;
  double c1, c2;
  uint32_t initcnt;
  lsl::stream_outlet* outlet;
  float lval;
  int baud;
  std::vector<double> range;
  uint32_t warncnt;
};

eog_t::eog_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_drawing_t(cfg),
    device("/dev/rfcomm1"),
    baudrate(38400),
    charsize(8),
    offset(512.0),
    scale(3.3/1024.0),
    //range(1.0),
    unit("mV"),
    dev(NULL),
    timeoutcnt(0),
    lval(0),
    baud(B9600),
    warncnt(0)
{
  range.push_back(0);
  range.push_back(1023);
  GET_ATTRIBUTE_(device);
  if( device.empty() )
    device = "/dev/rfcomm1";
  GET_ATTRIBUTE_(baudrate);
  GET_ATTRIBUTE_(charsize);
  GET_ATTRIBUTE_(offset);
  GET_ATTRIBUTE_(scale);
  GET_ATTRIBUTE_(range);
  if( range.size() != 2 )
    throw ErrMsg("Value range needs exactly two values.");
  GET_ATTRIBUTE_(unit);
  if( unit.empty() )
    unit = "mV";
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
  lsl::stream_info streaminfo(get_name(),modname,1,lsl::IRREGULAR_RATE,lsl::cf_double64);
  outlet = new lsl::stream_outlet(streaminfo);
  dev = new serialport_t();
  start_service();
}

eog_t::~eog_t() throw()
{
  stop_service();
  delete dev;
  delete outlet;
}

void eog_t::service()
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
      std::string resp(dev->readline(1000,'\n'));
      float x(0);
      if( sscanf(resp.c_str(),"%f",&x) == 1 ){
        timeoutcnt = 0;
        if( (x <= range[0]) || (x>=range[1]) )
          warncnt++;
        else
          warncnt = 0;
        if( warncnt == 1 )
          add_warning("At least one sample was saturated.");
        if( warncnt > 50 )
          add_critical("At least 50 consecutive samples were saturated.");
        x = (x - offset)*scale;
        lval = x;
        outlet->push_sample(&x);
        alive();
      }
    }
  }
  if( b_open )
    dev->close();
}

void eog_t::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
  cr->translate(0.5*width, 0.8*height );
  double wscale(0.5*std::min(height,width));
  double vscale(0.67);
  double r(1.3);
  double linewidth(2.0/wscale);
  cr->scale( wscale, wscale );
  cr->set_line_width( linewidth );
  cr->set_font_size( 6*linewidth );
  cr->set_source_rgb( 0, 0, 0 );
  cr->save();
  cr->arc( 0, 0, r, -vscale-TASCAR_PI2, vscale-TASCAR_PI2 );
  cr->stroke();
  cr->restore();
  for(int iv=-2;iv<=2;++iv){
    double v(0.5*iv);
    double x(r*sin(vscale*v));
    double y(r*cos(vscale*v));
    cr->move_to( 0.9*x, -0.9*y );
    cr->line_to( x, -y );
    char ctmp[32];
    sprintf(ctmp, "%1.1f", v );
    ctext_at(cr, 1.12*x, -1.12*y, ctmp );
    cr->stroke();
  }
  double v(lval);
  if( v != HUGE_VAL ){
    v *= vscale;
    cr->save();
    cr->set_source_rgb( 1.0, 0.2, 0.2 );
    cr->move_to( 0, 0 );
    cr->line_to( 1.2*sin( v ), -1.2*cos( v ) );
    cr->stroke();
    cr->restore();
  }
  ctext_at( cr, 0, 0.2, name + " / " + unit );
  cr->save();
  cr->arc(0,0,0.1,0,TASCAR_2PI);
  cr->fill();
  cr->restore();
}

REGISTER_SENSORPLUGIN(eog_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
