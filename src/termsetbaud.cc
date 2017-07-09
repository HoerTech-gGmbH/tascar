#include "termsetbaud.h"
#include <asm/termbits.h>
#include <sys/ioctl.h>

void term_setbaud( int fd, int baud )
{
  struct termios2 to;
  ioctl(fd, TCGETS2, &to);
  to.c_cflag &= ~CBAUD;
  to.c_cflag |= BOTHER;
  to.c_ispeed = baud;
  to.c_ospeed = baud;
  ioctl(fd, TCSETS2, &to);
}
