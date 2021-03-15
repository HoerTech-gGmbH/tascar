#include "dynamicobjects.h"
#include "cli.h"
#include <stdlib.h>
#include "errorhandling.h"

int main(int argc,char** argv)
{
  double geo_lon(0);
  double geo_lat(0);
  std::string gpxfile;
  bool znull(false);
  bool print_velocity(false);
  uint32_t nsmooth(0);
  double resample(0);
  // parse command line interface:
  const char *options = "hr:";
  struct option long_options[] = { 
    { "help", 0, 0, 'h' },
    { "lon", 1, 0, 'o' },
    { "lat", 1, 0, 'a' },
    { "znull", 0, 0, 'n' },
    { "resample", 1, 0, 'r' },
    { "smooth", 1, 0, 's' },
    { "velocity", 0, 0, 'v' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'h':
      TASCAR::app_usage("tascar_gpx2csv",long_options,"gpxfile");
      return 0;
    case 'o':
      geo_lon = atof(optarg);
      break;
    case 'a':
      geo_lat = atof(optarg);
      break;
    case 'r':
      resample = atof(optarg);
      break;
    case 'v':
      print_velocity = true;
      break;
    case 's':
      nsmooth = atoi(optarg);
      break;
    case 'n':
      znull = true;
      break;
    }
  }
  if( optind < argc )
    gpxfile = argv[optind++];
  if(gpxfile.empty())
    throw TASCAR::ErrMsg("Empty GPX file name.");
  TASCAR::track_t track;
  track.load_from_gpx( gpxfile );
  if( (geo_lon != 0) || (geo_lat != 0) ){
    TASCAR::pos_t p;
    p.set_sphere(R_EARTH,DEG2RAD*geo_lon,DEG2RAD*geo_lat);
    track.project_tangent( p );
  }
  if( znull )
    track *= TASCAR::pos_t( 1.0,1.0,0.0 );
  if( resample > 0 )
    track.resample( resample );
  if( nsmooth > 0 )
    track.smooth( nsmooth );
  track.shift_time( -track.t_min() );
  //track.prepare();
  if( print_velocity )
    std::cout << track.print_velocity(",");
  else
    std::cout << track.print_cart(",");
  std::cerr << "Track length is " << track.length() << " m.\nCenter is at "<< track.center().print_cart() << ".\n";
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
