/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <stdint.h>
#include <string>
#include <termios.h>

namespace TASCAR {

  class serialport_t {
  public:
    serialport_t();
    ~serialport_t();
    int open(const char* dev, int speed, int parity = 0, int stopbits = 0,
             bool xbaud = false);
    void set_interface_attribs(int speed, int parity, int stopbits, bool xbaud);
    void set_blocking(int should_block);
    bool isopen();
    std::string readline(uint32_t maxlen, char delim);
    void close();
    int fd;
  };

} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
