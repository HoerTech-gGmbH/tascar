/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2024 Giso Grimm
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

#include "../build/tascarver.h"
#include "cli.h"
#include "errorhandling.h"
#include <iostream>

int main(int argc, char** argv)
{
  const char* options = "h";
  struct option long_options[] = {{"help", 0, 0, 'h'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        EOF) {
    switch(opt) {
    case '?':
      throw TASCAR::ErrMsg("Invalid option.");
      break;
    case ':':
      throw TASCAR::ErrMsg("Missing argument.");
      break;
    case 'h':
      // usage(long_options);
      TASCAR::app_usage("tascar_sceneskeleton", long_options, "",
                        "Show a generic TASCAR scene skeleton.");
      return 0;
    }
  }

  std::cout << "<?xml version=\"1.0\"?>\n"
            << "<session license=\"CC0\">\n"
            << "  <!-- generated for version " << TASCARVER << " -->\n"
            << "  <scene>\n"
            << "  <!-- create your scene here -->\n"
            << "  <!-- \n"
            << "       <source name=\"in\">\n"
            << "         <sound>\n"
            << "           <plugins>\n"
            << "           </plugins>\n"
            << "         </sound>\n"
            << "       </source>\n"
            << "       <receiver name=\"out\" type=\"omni\"/>\n"
            << "       <facegroup name=\"walls\" shoebox=\"7 11 3\"/>\n"
            << "    --> \n"
            << "  </scene>\n"
            << "  <modules>\n"
            << "  </modules>\n"
            << "</session>\n";
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
