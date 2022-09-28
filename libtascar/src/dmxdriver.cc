/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include "dmxdriver.h"
#include "errorhandling.h"
#include "tscconfig.h"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace DMX;

driver_t::driver_t() {}

driver_t::~driver_t() {}

OpenDMX_USB_t::OpenDMX_USB_t(const char* device) : data_(&msg[1])
{
  msg[0] = 0;
  // open(device,B230400,0,2);
  open(device, 250000, 0, 2, true);
}

void OpenDMX_USB_t::send(uint8_t universe, const std::vector<uint16_t>& data)
{
  msg[0] = universe;
  msg[0] = 0;
  memset(data_, 0, 512);
  for(uint32_t k = 0; k < data.size(); ++k)
    data_[k] = data[k];
  ioctl(fd, TIOCSBRK, 0);
  usleep(110);
  ioctl(fd, TIOCCBRK, 0);
  usleep(16);
  uint32_t n(0);
  if((n = write(fd, msg, 513)) < 513) {
    TASCAR::add_warning("failed to write 513 bytes to DMX device");
  }
}

ArtnetDMX_t::ArtnetDMX_t(const char* hostname, const char* port)
    : data_(&(msg[18])), res(0)
{
  memset(msg, 0, 530);
  msg[0] = 'A';
  msg[1] = 'r';
  msg[2] = 't';
  msg[3] = '-';
  msg[4] = 'N';
  msg[5] = 'e';
  msg[6] = 't';
  msg[9] = 0x50;
  msg[11] = 14;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_ADDRCONFIG;
  int err(getaddrinfo(hostname, port, &hints, &res));
  if(err != 0)
    throw TASCAR::ErrMsg("failed to resolve remote socket address");
  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(fd == -1)
    throw TASCAR::ErrMsg(strerror(errno));
}

void ArtnetDMX_t::send(uint8_t universe, const std::vector<uint16_t>& data)
{
  msg[14] = universe;
  msg[16] = data.size() >> 8;
  msg[17] = data.size() & 0xff;
  for(uint32_t k = 0; k < data.size(); ++k)
    data_[k] = data[k];
  if(sendto(fd, msg, 18 + data.size(), 0, res->ai_addr, res->ai_addrlen) == -1)
    throw TASCAR::ErrMsg(strerror(errno));
}

OSC_t::OSC_t(const char* hostname, const char* port, uint16_t maxchannel,
             const std::string& path)
    : target(lo_address_new(hostname, port)), msg(lo_message_new()), path_(path)
{
  for(uint16_t k = 0; k < maxchannel; ++k)
    lo_message_add_int32(msg, 0);
  argc = lo_message_get_argc(msg);
  argv = lo_message_get_argv(msg);
}

void OSC_t::send(uint8_t, const std::vector<uint16_t>& data)
{
  for(uint32_t k = 0; k < std::min((uint32_t)(data.size()), (uint32_t)argc);
      ++k)
    argv[k]->i = data[k];
  lo_send_message(target, path_.c_str(), msg);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
