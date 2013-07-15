#ifndef DEFS_H
#define DEFS_H

/**
   \file defs.h
   \brief Some basic definitions
*/

#define PI2 6.283185307179586232
#define PI_2 1.570796326794896558
#define DEG2RAD 0.017453292519943295474
#define RAD2DEG 57.29577951308232286464
#define EPS 3.0e-6

#define PI2f 6.283185307179586232f
#define PI_2f 1.570796326794896558f
#define DEG2RADf 0.017453292519943295474f
#define RAD2DEGf 57.29577951308232286464f
#define EPSf 3.0e-6f


/** 
    \brief average radius of earth in meters:
*/
#define R_EARTH 6367467.5


#define DEBUG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << " " << #x << "=" << x << std::endl
//#define DEBUGMSG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << x << std::endl
#define DEBUGMSG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << " --" << x << "--" << std::endl

/**
   \defgroup libtascar TASCAR core library

   This library is published under the LGPL.
*/
/**
   \defgroup apptascar TASCAR applications

   This set of applications is published under the GPL.
*/
/**
   \defgroup acousticmodel The acoustic model

   Two types of sources: omnidirectional point source
   (TASCAR::Acousticmodel::pointsource_t) and first order ambisonics
   (not yet implemented).

   Multiple types of sinks (TASCAR::Acousticmodel::sink_t):
   omnidirectional single channel sink
   (TASCAR::Acousticmodel::sink_omni_t), 3rd (?) order full periphonic
   sink (not yet implemented), 11th order horizontal sink (not yet
   implemented), Duda head model binaural sink (not yet implemented),
   omnidirectional room sink. Each sink has a reference point, which
   can depend on the source position.

   Each pair of a source and any sink owns an acoustic model
   (TASCAR::Acousticmodel::acoustic_model_t). Distance coding
   (distance decay, air absorption) is processed by the acoustic
   model, which is calculated independently of the type of sink.

   Obstacles (TASCAR::Acousticmodel::obstacle_t) modify the position
   and frequency response of a source if they intersect with the
   direct connection between source and sink. (not yet implemented)

   Reflector objects (TASCAR::Acousticmodel::reflector_t) create one
   mirror source (TASCAR::Acousticmodel::mirrorsource_t) for each
   primary source, which use the audio material of a parent source,
   but have a separate position. An obstacle is placed around the
   reflector to process the spatial limitation of the mirror (not yet
   implemented).

 */

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
