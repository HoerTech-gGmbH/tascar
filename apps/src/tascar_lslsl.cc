/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

// gets rid of annoying "deprecated conversion from string constant blah blah" warning
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "cli.h"
#include <iostream>
#include <lsl_c.h>

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
      TASCAR::app_usage("tascar_lslsl", long_options, "", "List LSL streams.");
      return 0;
    }
  }
  unsigned buffer_elements(1024);
  lsl_streaminfo buffer[buffer_elements];
  int num_streams(lsl_resolve_all(buffer, buffer_elements, 1));
  for(int kstream = 0; kstream < num_streams; kstream++) {
    std::string fmt("undefined");
    switch(lsl_get_channel_format(buffer[kstream])) {
    case cft_float32:
      fmt = "float32";
      break;
    case cft_double64:
      fmt = "double64";
      break;
    case cft_string:
      fmt = "string";
      break;
    case cft_int32:
      fmt = "int32";
      break;
    case cft_int16:
      fmt = "int16";
      break;
    case cft_int8:
      fmt = "int8";
      break;
    case cft_int64:
      fmt = "int64";
      break;
    default:
      fmt = "undefined";
      break;
    }
    std::cout << lsl_get_name(buffer[kstream])
              << " type=" << lsl_get_type(buffer[kstream]) << " fmt=" << fmt
              << " channels=" << lsl_get_channel_count(buffer[kstream])
              << " fs=" << lsl_get_nominal_srate(buffer[kstream]) << " Hz " <<
        // lsl_get_uid(buffer[kstream]) << "@" <<
        lsl_get_hostname(buffer[kstream]) << " ("
              << lsl_get_source_id(buffer[kstream]) << ")" << std::endl;
    lsl_destroy_streaminfo(buffer[kstream]);
  }
  if(num_streams == 0)
    std::cout << "no LSL streams found." << std::endl;
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
