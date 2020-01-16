#include "cli.h"
#include <iostream>
#include "../build/tascarver.h"

void TASCAR::app_usage(const std::string& app_name,struct option * opt,const std::string& app_arg, const std::string& help)
{
  std::cout << "Usage:\n\n" << app_name << " [options] " << app_arg << "\n\n";
  if( !help.empty() )
    std::cout << help << "\n\n";
  std::cout << "Options:\n\n";
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
