#include "../build/tascarver.h"
#include <iostream>
#include "cli.h"

int main(int argc, char** argv)
{
  const char* options = "h";
  struct option long_options[] = {{"help", 0, 0, 'h'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        EOF) {
    switch(opt) {
    case 'h':
      // usage(long_options);
      TASCAR::app_usage("tascar_version", long_options, "", "Show version information.");
      return 0;
    }
  }

  std::cout << TASCARVER << std::endl;
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
