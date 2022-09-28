/**
 * @file   dmxdriver.h
 * @author Giso Grimm
 *
 * @brief  Driver class for ArtnetDMX and OpenDMX_USB for light control
 */
/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
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
#ifndef DMXDRIVER_H
#define DMXDRIVER_H

#include "serialport.h"
#include <netdb.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#if defined(_WIN32) && !defined(WIN32)
#define WIN32 _WIN32 // liblo needs WIN32 defined in order to detect Windows
#endif
#include <lo/lo.h>

/// classes for DMX based light control
namespace DMX {

  class driver_t {
  public:
    driver_t();
    virtual ~driver_t();
    virtual void send(uint8_t universe, const std::vector<uint16_t>& data) = 0;
  };

  class OpenDMX_USB_t : public driver_t, public TASCAR::serialport_t {
  public:
    OpenDMX_USB_t(const char* device);
    void send(uint8_t universe, const std::vector<uint16_t>& data);

  private:
    uint8_t msg[513];
    uint8_t* data_;
  };

  class ArtnetDMX_t : public driver_t {
  public:
    ArtnetDMX_t(const char* hostname, const char* port);
    void send(uint8_t universe, const std::vector<uint16_t>& data);

  private:
    uint8_t msg[530];
    uint8_t* data_;
    struct addrinfo* res;
    int fd;
  };

  class OSC_t : public driver_t {
  public:
    OSC_t(const char* hostname, const char* port, uint16_t maxchannel,
          const std::string& path);
    void send(uint8_t universe, const std::vector<uint16_t>& data);

  private:
    lo_address target;
    lo_message msg;
    int argc;
    lo_arg** argv;
    std::string path_;
  };

} // namespace DMX

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
