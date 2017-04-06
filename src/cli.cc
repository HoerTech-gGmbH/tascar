#include "cli.h"
#include <iostream>
#include "../build/tascarver.h"

void TASCAR::app_usage(const std::string& app_name,struct option * opt,const std::string& app_arg)
{
  std::cout << "Usage:\n\n" << app_name << " [options] " << app_arg << "\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
  std::cout << std::endl << "(version: " << TASCARVER << ")" << std::endl;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
