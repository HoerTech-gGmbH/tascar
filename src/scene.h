/**
   \file scene.h
   \brief "scene" provide classes for scene definition, without rendering functionality
   
   \ingroup libtascar
   \author Giso Grimm
   \date 2013

   \section license License (LGPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/
#ifndef SCENE_H
#define SCENE_H

#include "coordinates.h"
#include <string>
#include <vector>

namespace TASCAR {

  class sound_t {
  public:
    sound_t();
    std::string filename;
    double gain;
    unsigned int channel;
    unsigned int loop;
  };

  class bg_amb_t {
  public:
    bg_amb_t();
    double start;
    std::string filename;
    double gain;
    unsigned int loop;
  };

  class src_object_t {
  public:
    src_object_t();
    std::string name;
    double start;
    sound_t sound;
    TASCAR::track_t position;
  };

  class listener_t : public TASCAR::track_t {
  public:
    listener_t();
    TASCAR::track_t position;
  };

  class scene_t {
  public:
    scene_t();
    std::string name;
    double lat;
    double lon;
    double elev;
    std::vector<src_object_t> src;
    std::vector<bg_amb_t> bg_amb;
    listener_t listener;
  };

  scene_t xml_read_scene(const std::string& filename);
  void xml_write_scene(const std::string& filename, const scene_t& scene);

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
