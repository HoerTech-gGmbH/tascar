/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef DEFS_H
#define DEFS_H

#include <iostream>
#include <limits>

#ifdef HAS_APPLE_UNIFIED_LOG_
#include <os/log.h>
#endif

#ifdef HAS_SYSLOG_
#include <syslog.h>
#endif


/**
   \file defs.h
   \brief Some basic definitions
*/

#define TASCAR_PI  3.1415926535897932 ///< double value closest to pi
#define TASCAR_2PI 6.2831853071795865 ///< double value closest to two pi
#define TASCAR_PI2 1.5707963267948966 ///< double value closest to half pi
#define DEG2RAD 0.017453292519943296  ///< double value closest to pi / 180
#define RAD2DEG 57.295779513082321    ///< double value closest to 180 / pi

// Let compiler check expectations for PI related definitions
static_assert(TASCAR_PI * 2 == TASCAR_2PI and TASCAR_PI / 2 == TASCAR_PI2);
static_assert(TASCAR_PI/180 == DEG2RAD and 180/TASCAR_PI == RAD2DEG);
static_assert(1 == DEG2RAD * RAD2DEG);
#ifdef M_PI // for source files which include math header before this one
static_assert(M_PI == TASCAR_PI);
#endif

#define TASCAR_PIf  3.1415927f   ///< float value closest to pi
#define TASCAR_2PIf 6.2831853f   ///< float value closest to two pi
#define TASCAR_PI2f 1.57079633f  ///< float value closest to half pi
#define DEG2RADf    0.017453293f ///< float value closest to pi / 180 
#define RAD2DEGf   57.295780f    ///< float value closest to 180 / pi

// Let compiler check expectations for PIf related definitions
static_assert((float)TASCAR_PI == TASCAR_PIf);
static_assert((float)TASCAR_2PI == TASCAR_2PIf);
static_assert((float)TASCAR_PI2 == TASCAR_PI2f);
static_assert((float)DEG2RAD == DEG2RADf);
static_assert((float)RAD2DEG == RAD2DEGf);
static_assert(TASCAR_PIf * 2 == TASCAR_2PIf and TASCAR_PIf / 2 == TASCAR_PI2f);
static_assert(TASCAR_PIf/180 == DEG2RADf);
static_assert(1 == DEG2RADf * RAD2DEGf);
// division 180/pi produces a rounding difference when done in single precision
static_assert(180/TASCAR_PIf >= RAD2DEGf*(1-std::numeric_limits<float>::epsilon()));
static_assert(180/TASCAR_PIf <= RAD2DEGf*(1+std::numeric_limits<float>::epsilon()));
static_assert((float)(180/TASCAR_PI) == RAD2DEGf);

// Mark short pi-related symbols deprecated in favor of the macros beginning with
// TASCAR_. Old symbols will be unavailable in a future version of libtascar.

/// constant for pi @deprecated use TASCAR_PIf instead
[[deprecated("use TASCAR_PIf instead")]] constexpr float PIf = TASCAR_PIf;
/// constant for two pi @deprecated use TASCAR_2PI instead
[[deprecated("use TASCAR_2PI instead")]] constexpr double PI2 = TASCAR_2PI;
/// constant for two pi @deprecated use TASCAR_2PIf instead
[[deprecated("use TASCAR_2PIf instead")]] constexpr float PI2f = TASCAR_2PIf;
/// constant for half pi @deprecated use TASCAR_PI2 instead
[[deprecated("use TASCAR_PI2 instead")]] constexpr double PI_2 = TASCAR_PI2;
/// constant for half pi @deprecated use TASCAR_PI2f instead
[[deprecated("use TASCAR_PI2f instead")]] constexpr float PI_2f = TASCAR_PI2f;

#define EPS 3.0e-6
#define EPSf 3.0e-6f

#define SPLREF -93.9794
#define SPLREFf -93.9794f



/**
    \brief average radius of earth in meters:
*/
#define R_EARTH 6367467.5

#ifdef HAS_APPLE_UNIFIED_LOG_
/**
    Use os_log
 */
#ifndef DEBUGS
#define DEBUGS os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_DEBUG, "%", (std::to_string(x)).c_str())
#endif
#ifndef DEBUG
#define DEBUG os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_DEBUG, "%", (std::to_string(x)).c_str())
#endif
#ifndef DEBUGMSG
#define DEBUGMSG os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_DEBUG, "%", (std::to_string(x)).c_str())
#endif
#elif HAS_SYSLOG
/**
    Use syslog
 */
#ifndef DEBUGS
#define DEBUGS syslog(get_syslog_priority(metadata.severity), "%s", (std::to_string(x)).c_str());
#endif
#ifndef DEBUG
#define DEBUG syslog(get_syslog_priority(metadata.severity), "%s", (std::to_string(x)).c_str());
#endif
#ifndef DEBUGMSG
#define DEBUGMSG syslog(get_syslog_priority(metadata.severity), "%s", (std::to_string(x)).c_str());
#endif
#else
/**
    Use default std error output
 */
#ifndef DEBUGS
#define DEBUGS(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x << std::endl
#endif
#ifndef DEBUG
#define DEBUG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << " " << #x << "=" << x << std::endl
#endif
#ifndef DEBUG2
#define DEBUG2(x,y) std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << " " << #x << "=" << x << ", " << #y << "=" << y << std::endl
#endif
#ifndef DEBUGMSG
#define DEBUGMSG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << " --" << x << "--" << std::endl
#endif
#endif


#define TASCAR_ASSERT(x) if( !(x) ) throw TASCAR::ErrMsg(std::string(__FILE__)+":"+std::to_string(__LINE__)+": Expression " #x " is false.")

#define TASCAR_ASSERT_EQ(x,y) if( !(x==y) ) throw TASCAR::ErrMsg(#x "!=" #y " (x="+TASCAR::to_string(x)+", y="+TASCAR::to_string(y)+")"+__FILE__+":"+TASCAR::to_string(__LINE__))

/**
   \defgroup libtascar TASCAR core library

*/
/**
   \defgroup acousticmodel The acoustic model

   Two types of sources: omnidirectional point source
   (TASCAR::Acousticmodel::pointsource_t) and first order ambisonics
   (not yet implemented).

   Multiple types of receivers (TASCAR::Acousticmodel::receiver_t):
   omnidirectional single channel receiver
   (TASCAR::Acousticmodel::receiver_omni_t), 3rd (?) order full periphonic
   receiver (not yet implemented), 11th order horizontal receiver (not yet
   implemented), Duda head model binaural receiver (not yet implemented),
   omnidirectional room receiver. Each receiver has a reference point, which
   can depend on the source position.

   Each pair of a source and any receiver owns an acoustic model
   (TASCAR::Acousticmodel::acoustic_model_t). Distance coding
   (distance decay, air absorption) is processed by the acoustic
   model, which is calculated independently of the type of receiver.

   Obstacles (TASCAR::Acousticmodel::obstacle_t) modify the position
   and frequency response of a source if they intersect with the
   direct connection between source and receiver. (not yet implemented)

   Reflector objects (TASCAR::Acousticmodel::reflector_t) create one
   mirror source (TASCAR::Acousticmodel::mirrorsource_t) for each
   primary source, which use the audio material of a parent source,
   but have a separate position. An obstacle is placed around the
   reflector to process the spatial limitation of the mirror (not yet
   implemented).

   A scene (TASCAR::Scene::scene_t) contains all spatial information.

   What happens in a process cycle?
   <ol>
   <li>Update audio of each primary source</li>
   <li>Update position of each primary source</li>
   <li>Update position/orientation of all obstacles and reflectors</li>
   <li>Update position/orientation of all receivers</li>
   <li>Process world</li>
   <li>Copy rendered audio from receivers to audio backend</li>
   </ol>

   Relevant name spaces:
   <ul>
   <li>TASCAR::Scene</li>
   <li>TASCAR::Acousticmodel</li>
   </ul>

 */
/**
   \defgroup callgraph The call-graph of audio processing

   The top-level processing method is TASCAR::render_rt_t::process(),
   which renders the audio for one acoustic scene. Each scene is
   rendered independently.

   <ul>
   <li>Geometry processing TASCAR::Scene::scene_t::geometry_update()</li>
   <li>Activity processing TASCAR::Scene::scene_t::process_active()</li>
   <li>Acoustic modeling TASCAR::Acousticmodel::world_t::process()</li>
   <ul>
   <li>Image source model TASCAR::Acousticmodel::mirror_model_t::process(), TASCAR::Acousticmodel::mirrorsource_t::process()</li>
   <li>Point source (primary and image source) transmission model TASCAR::Acousticmodel::acoustic_model_t::process()</li>
   <ul>
   <li>Receiver specific panning TASCAR::Acousticmodel::receiver_t::add_pointsource()</li>
   </ul>
   <li>Diffuse source model TASCAR::Acousticmodel::diffuse_acoustic_model_t::process()</li>
   <ul>
   <li>Receiver specific FOA decoding TASCAR::Acousticmodel::receiver_t::add_diffuse_sound_field()</li>
   </ul>
   <li>Output gain mask processing</li>
   </ul>
   </ul>
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
