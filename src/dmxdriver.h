#ifndef DMXDRIVER_H
#define DMXDRIVER_H

#include <stdint.h>
#include <vector>
#include "serialport.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace DMX {

  class driver_t {
  public:
    driver_t();
    virtual ~driver_t();
    virtual void send(uint8_t universe, const std::vector<uint16_t>& data) = 0;
  };

  class OpenDMX_USB_t : public driver_t, public TASCAR::serialport_t {
  public:
    OpenDMX_USB_t( const char* device );
    void send( uint8_t universe, const std::vector<uint16_t>& data );
  private:
    uint8_t msg[513];
    uint8_t* data_;
  };

  class ArtnetDMX_t : public driver_t {
  public:
    ArtnetDMX_t( const char* hostname, const char* port );
    void send( uint8_t universe, const std::vector<uint16_t>& data );
  private:
    uint8_t msg[530];
    uint8_t* data_;
    struct addrinfo* res;
    int fd;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
