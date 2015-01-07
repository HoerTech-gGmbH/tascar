#ifndef CLI_H
#define CLI_H

#include <string>
#include <getopt.h>

namespace TASCAR {

  void app_usage(const std::string& app_name,struct option * opt,const std::string& app_arg="");

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
