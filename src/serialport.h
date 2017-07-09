#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <string>
#include <stdint.h>
#include <termios.h>

namespace TASCAR {

  class serialport_t {
  public:
    serialport_t();
    ~serialport_t();
    int open( const char* dev, int speed, int parity = 0, int stopbits = 0, bool xbaud=false);
    void set_interface_attribs( int speed, int parity, int stopbits, bool xbaud );
    void set_blocking( int should_block );
    bool isopen();
    std::string readline(uint32_t maxlen,char delim);
    void close();
  protected:
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
