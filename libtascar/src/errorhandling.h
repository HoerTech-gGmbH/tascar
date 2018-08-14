#ifndef ERRORHANDLING_H
#define ERRORHANDLING_H

#include <string>

namespace TASCAR {

  class ErrMsg : public std::exception, private std::string {
  public:
    ErrMsg(const std::string& msg);
    virtual ~ErrMsg() throw();
    const char* what() const throw();
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
