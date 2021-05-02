/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "serialport.h"
#include "errorhandling.h"
#include <string.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/ioctl.h>
#include "termsetbaud.h"

using namespace TASCAR;

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

int serialport_t::open( const char* dev, int speed, int parity, int stopbits, bool xbaud )
{
  fd = ::open( dev, O_RDWR | O_NOCTTY | O_SYNC );
  if( fd < 0 )
    throw TASCAR::ErrMsg(std::string("Unable to open device ")+dev);
  set_interface_attribs( speed, parity, stopbits, xbaud );
  set_blocking( 1 );
  return fd;
}

void serialport_t::set_interface_attribs( int speed, int parity, int stopbits, bool xbaud )
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr(fd, &tty) != 0)
    throw TASCAR::ErrMsg("Error from tcgetattr");
  if( !xbaud ){
    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);
  }
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
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff flow ctrl
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
  if( xbaud )
    term_setbaud( fd, speed );
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

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
