/**
   \file tascar_gpx2csv.cc
   \brief Convert gpx track into local coordinates in csv format
   \ingroup apptascar
   \author Giso Grimm
   \date 2012

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
#include <libxml++/libxml++.h>
#include <iostream>
#include "tascar.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <unistd.h>
#include <getopt.h>

using namespace TASCAR;

namespace TASCAR {


  TASCAR::track_t track_transform( TASCAR::track_t track, const std::string& cfgname )
  {
    xmlpp::DomParser parser(cfgname);
    xmlpp::Element* root(parser.get_document()->get_root_node());
    track.edit(root->get_children());
    return track;
  }

}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_gpx2csv [options] input\n\n  input: gpx file\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc,char** argv)
{
  const char *options = "c:hps:";
  struct option long_options[] = { 
    { "config",    1, 0, 'c' },
    { "help",      0, 0, 'h' },
    { "starttime", 1, 0, 's' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  double starttime(0.0);
  std::string configfile;
  std::string infile;
  bool projectcenter(false);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != EOF){
    switch(opt){
    case 'c':
      configfile = optarg;
      break;
    case 'h':
      usage(long_options);
      return -1;
    case 'p':
      projectcenter = true;
      break;
    case 's':
      starttime = atof(optarg);
      break;
    }
  }
  if( optind < argc )
    infile = argv[optind++];
  if( !infile.size() ){
    std::cerr << "Error: input file name missing\n";
    return -1;
  }
  TASCAR::track_t track;
  track.load_from_gpx( infile );
  if( projectcenter )
    track.project_tangent( track.center() );
  if( configfile.size() )
    track = TASCAR::track_transform( track, configfile );
  track.shift_time(starttime - track.begin()->first);
  track.resample(0.5);
  track.smooth(7);
  TASCAR::track_t::iterator it,itp(track.begin());
  for( it=track.begin();it!=track.end();++it){
    if( (it->first != itp->first) && (it->first >= 0) ){
      double dt = it->first - itp->first;
      std::cout << it->first << "," << TASCAR::distance(itp->second,it->second)/dt << std::endl;
    }
    itp=it;
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
