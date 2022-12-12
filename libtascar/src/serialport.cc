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
#include "defs.h"
#include "errorhandling.h"
#include "termsetbaud.h"
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace TASCAR;

#define TESTFLAG(x, y)                                                         \
  if(x & y)                                                                    \
  str += std::string(#y) + std::string(" ")

std::string test_iflag(int flag)
{
  std::string str = std::to_string(flag);
  str += "\n";
  TESTFLAG(flag, IGNBRK);
  TESTFLAG(flag, BRKINT);
  TESTFLAG(flag, IGNPAR);
  TESTFLAG(flag, PARMRK);
  TESTFLAG(flag, INPCK);
  TESTFLAG(flag, ISTRIP);
  TESTFLAG(flag, INLCR);
  TESTFLAG(flag, IGNCR);
  TESTFLAG(flag, ICRNL);
  TESTFLAG(flag, IXON);
  TESTFLAG(flag, IXANY);
  TESTFLAG(flag, IXOFF);
  TESTFLAG(flag, IMAXBEL);
  TESTFLAG(flag, IUTF8);
  return str;
}

std::string test_lflag(int flag)
{
  std::string str = std::to_string(flag);
  str += "\n";
  TESTFLAG(flag, ISIG);
  TESTFLAG(flag, ICANON);
  TESTFLAG(flag, ECHO);
  TESTFLAG(flag, ECHOE);
  TESTFLAG(flag, ECHOK);
  TESTFLAG(flag, ECHONL);
  TESTFLAG(flag, NOFLSH);
  TESTFLAG(flag, TOSTOP);
  TESTFLAG(flag, ECHOCTL);
  TESTFLAG(flag, ECHOPRT);
  TESTFLAG(flag, ECHOKE);
  TESTFLAG(flag, FLUSHO);
  TESTFLAG(flag, PENDIN);
  TESTFLAG(flag, IEXTEN);
  TESTFLAG(flag, EXTPROC);
  return str;
}

#define TESTBAUD(flag, y)                                                      \
  if((flag & 0x100F) == y)                                                     \
  str += std::string(#y) + std::string(" ")

std::string test_cflag(int flag)
{
  std::string str = std::to_string(flag);
  str += "\n";

  TESTBAUD(flag, B0);
  TESTBAUD(flag, B50);
  TESTBAUD(flag, B75);
  TESTBAUD(flag, B110);
  TESTBAUD(flag, B134);
  TESTBAUD(flag, B150);
  TESTBAUD(flag, B200);
  TESTBAUD(flag, B300);
  TESTBAUD(flag, B600);
  TESTBAUD(flag, B1200);
  TESTBAUD(flag, B1800);
  TESTBAUD(flag, B2400);
  TESTBAUD(flag, B4800);
  TESTBAUD(flag, B9600);
  TESTBAUD(flag, B19200);
  TESTBAUD(flag, B38400);
  TESTBAUD(flag, B115200);
  TESTBAUD(flag, B230400);
  TESTFLAG(flag, CSTOPB);
  TESTFLAG(flag, CREAD);
  TESTFLAG(flag, PARENB);
  TESTFLAG(flag, PARODD);
  TESTFLAG(flag, HUPCL);
  TESTFLAG(flag, CLOCAL);
  TESTFLAG(flag, CRTSCTS);
  return str;
}

serialport_t::serialport_t() : fd(-1) {}

serialport_t::~serialport_t()
{
  if(fd > 0)
    close();
}

bool serialport_t::isopen()
{
  return (fd > 0);
}

int serialport_t::open(const char* dev, int speed, int parity, int stopbits,
                       bool xbaud)
{
#ifdef ISMACOS
  fd = ::open(dev, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
#else
  fd = ::open(dev, O_RDWR | O_NOCTTY | O_SYNC);
#endif
  if(fd < 0)
    throw TASCAR::ErrMsg(std::string("Unable to open device ") + dev);
  set_interface_attribs(speed, parity, stopbits, xbaud);
  set_blocking(1);
  return fd;
}

void serialport_t::set_interface_attribs(int speed, int parity, int stopbits,
                                         bool xbaud)
{
  struct termios tty;
  memset(&tty, 0, sizeof tty);
  if(tcgetattr(fd, &tty) != 0)
    throw TASCAR::ErrMsg("Error from tcgetattr");
  if(!xbaud) {
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
  }
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK; // disable break processing
  tty.c_iflag |= BRKINT;  // enable break processing
  tty.c_iflag |= ICRNL;
  tty.c_lflag = 0; // no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0; // no remapping, no delays
#ifdef ISMACOS
  tty.c_oflag &= ~OPOST;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
#endif
  tty.c_cc[VMIN] = 0;                     // read doesn't block
  tty.c_cc[VTIME] = 5;                    // 0.5 seconds read timeout
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff flow ctrl
  tty.c_cflag |= (CLOCAL | CREAD);        // ignore modem controls,
  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  if(stopbits == 2)
    tty.c_cflag |= CSTOPB;
  tty.c_cflag &= ~CRTSCTS;
  // DEBUG(test_iflag(tty.c_iflag));
  // DEBUG(test_lflag(tty.c_lflag));
  // DEBUG(test_cflag(tty.c_cflag));
  if(tcsetattr(fd, TCSANOW, &tty) != 0)
    throw TASCAR::ErrMsg("error from tcsetattr");
  int flags;
  ioctl(fd, TIOCMGET, &flags);
  flags &= ~TIOCM_RTS;
  flags &= ~(TIOCM_RTS | TIOCM_DTR);
  // DEBUG(flags);
  ioctl(fd, TIOCMSET, &flags);
  if(xbaud)
    term_setbaud(fd, speed);
}

void serialport_t::set_blocking(int should_block)
{
  struct termios tty;
  memset(&tty, 0, sizeof tty);
  if(tcgetattr(fd, &tty) != 0)
    throw TASCAR::ErrMsg("error from tggetattr");
  tty.c_cc[VMIN] = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout
  if(tcsetattr(fd, TCSANOW, &tty) != 0)
    throw TASCAR::ErrMsg("error setting term attributes");
}

void serialport_t::close()
{
  ::close(fd);
}

std::string serialport_t::readline(uint32_t maxlen, char delim)
{
  std::string r;
  while(isopen() && maxlen) {
    maxlen--;
    char c(0);
    if(::read(fd, &c, 1) == 1) {
      if(c != delim)
        r += c;
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
