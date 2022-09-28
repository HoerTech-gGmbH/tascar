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

#include "termsetbaud.h"
#ifdef ISLINUX
#include <asm/termbits.h>
#endif
#ifdef ISMACOS
#include  <termios.h>
#include  <IOKit/serial/ioss.h>
#endif
#include <sys/ioctl.h>

void term_setbaud(int fd, int baud)
{
#ifdef ISLINUX
  struct termios2 to;
  ioctl(fd, TCGETS2, &to);
  to.c_cflag &= ~CBAUD;
  to.c_cflag |= BOTHER;
  to.c_ispeed = baud;
  to.c_ospeed = baud;
  ioctl(fd, TCSETS2, &to);
#endif
#ifdef ISMACOS
  speed_t speed = baud;
  ioctl(fd, IOSSIOSPEED, &speed);
#endif
}
