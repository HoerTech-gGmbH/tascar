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
   \defgroup apphos Applications specific to the Harmony of the Spheres project

   This set of applications is published under the GPL and free licenses.
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
