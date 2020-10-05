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
