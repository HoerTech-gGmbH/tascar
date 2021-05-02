/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include <jack/jack.h>
#include <iostream>
#include "defs.h"
#include "cli.h"

int main(int argc, char** argv)
{
  const char *options = "hj:oips";
  struct option long_options[] = { 
    { "help",     0, 0, 'h' },
    { "jackname", 1, 0, 'j' },
    { "output",   0, 0, 'o' },
    { "input",    0, 0, 'i' },
    { "physical", 0, 0, 'p' },
    { "soft",     0, 0, 's' },
    { 0, 0, 0, 0 }
  };
  std::string jackname("tascar_lsjackp");
  bool soft(false);
  int opt(0);
  int option_index(0);
  int jackflags(0);
  while( (opt = getopt_long(argc, argv, options,
			    long_options, &option_index)) != -1){
    switch(opt){
    case 'h':
        TASCAR::app_usage("tascar_lsjackp",long_options,"");
        return 0;
    case 'j':
      jackname = optarg;
      break;
    case 'o':
      jackflags |= JackPortIsOutput;
      break;
    case 'i':
      jackflags |= JackPortIsInput;
      break;
    case 'p':
      jackflags |= JackPortIsPhysical;
      break;
    case 's':
      soft = true;
      break;
    }
  }
  jack_client_t* jc(jack_client_open(jackname.c_str(),
				     JackNoStartServer,
				     NULL));
  if( jc ){
    const char** ports(jack_get_ports(jc,
				      NULL,
				      NULL,
				      jackflags));
    if( ports ){
      while( *ports ){
	jack_port_t* p(jack_port_by_name(jc,*ports));
	if( p ){
	  if( (!soft) || ((jack_port_flags( p ) & JackPortIsPhysical) == 0) )
	    std::cout << *ports << std::endl;
	//jack_free( *ports );
	}
	++ports;
      }
    }
  }
  return 0;
}
