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

#define SPLREF -93.9794
#define SPLREFf -93.9794f


/**
    \brief average radius of earth in meters:
*/
#define R_EARTH 6367467.5

#ifndef DEBUGS
#define DEBUGS(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x << std::endl
#endif

#ifndef DEBUG
#define DEBUG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << " " << #x << "=" << x << std::endl
#endif

#ifndef DEBUGMSG
#define DEBUGMSG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << " --" << x << "--" << std::endl
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
