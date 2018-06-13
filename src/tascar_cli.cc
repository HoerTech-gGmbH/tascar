/**
   \file tascar_cli.cc
   \ingroup apptascar
   \brief TASCAR command line interface
   \author Giso Grimm, Carl-von-Ossietzky Universitaet Oldenburg
   \date 2013, 2014

   \section license License (GPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

/*
  todo: diffuse reverb for enclosed sources, damping+attenuation for
  non-enclosed sources
  
  todo: directive sound sources - normal vector and size define
  directivity

*/

#include "session.h"
#include "jackiowav.h"
#include "cli.h"

static bool b_quit;

static void sighandler(int sig)
{
  b_quit = true;
  fclose(stdin);
}

int main(int argc, char** argv)
{
  try{
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    std::string cfgfile;
    std::string jackname;
    std::string output;
    std::string range;
    bool showlicense(false);
    bool use_range(false);
    bool validate(false);
    const char *options = "hj:o:r:lv";
    struct option long_options[] = { 
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'j' },
      { "output",   1, 0, 'o' },
      { "range",    1, 0, 'r' },
      { "licenses", 0, 0, 'l' },
      { "validate", 0, 0, 'v' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'h':
        TASCAR::app_usage("tascar_cli",long_options,"configfile");
        return -1;
      case 'j':
        jackname = optarg;
        break;
      case 'o':
        output = optarg;
        break;
      case 'l':
        showlicense = true;
        break;
      case 'v':
        validate = true;
        break;
      case 'r':
        range = optarg;
        use_range = true;
        break;
      }
    }
    if( optind < argc )
      cfgfile = argv[optind++];
    if( cfgfile.size() == 0 ){
      TASCAR::app_usage("tascar_cli",long_options,"configfile");
      return -1;
    }
    TASCAR::session_t session(cfgfile,TASCAR::xml_doc_t::LOAD_FILE,cfgfile);
    if( validate ){
      session.start();
      sleep(1);
      session.stop();
      std::string v;
      if( v.size() && TASCAR::warnings.size() )
        v+="\n";
      for(std::vector<std::string>::const_iterator it=TASCAR::warnings.begin();it!=TASCAR::warnings.end();++it){
        v+= "Warning: " + *it + "\n";
      }
      session.validate_attributes(v);
      if( v.empty() )
        return 0;
      else{
        std::cerr << v << std::endl;
        return 1;
      }
    }
    if( showlicense ){
      std::cout << session.legal_stuff() << std::endl;
      return 0;
    }
    if( range.empty() && use_range ){
      for(uint32_t k=0;k<session.ranges.size();k++)
        std::cout << session.ranges[k]->name << std::endl;
      return 0;
    }
    if( output.empty() )
      session.run(b_quit);
    else{
      double t_start(0);
      double t_dur(session.get_duration());
      bool range_found(false);
      if( use_range ){
        for(uint32_t k=0;k<session.ranges.size();k++)
          if( session.ranges[k]->name == range ){
            t_start = session.ranges[k]->start;
            t_dur = session.ranges[k]->end-session.ranges[k]->start;
            range_found = true;
          }
        if( !range_found )
          throw TASCAR::ErrMsg("Range \""+range+"\" not found.");
      }
      session.start();
      std::vector<std::string> ports(session.get_render_output_ports());
      std::cout << "Recording " << ports.size() << " channels, " << t_dur << " seconds to \"" << output << "\":" << std::endl;
      for(uint32_t k=0;k<ports.size();k++)
        std::cout << k << " " << ports[k] << std::endl;
      jackio_t* iow(new jackio_t(t_dur,output,ports,session.name+"jackio"));
      iow->set_transport_start(t_start,false);
      iow->run();
      std::cout << iow->get_xruns() << " xruns (total latency: " << iow->get_xrun_latency() << ")" << std::endl;
      delete iow;
      sleep(1);
      session.stop();
    }
  }
  catch( const std::exception& msg ){
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch( const char* msg ){
    std::cerr << "Error: " << msg << std::endl;
    return 1;
  }
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
