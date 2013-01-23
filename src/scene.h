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
#include "async_file.h"
#include <string>
#include <vector>
#include <libxml++/libxml++.h>

namespace TASCAR {

  class scene_node_base_t {
  public:
    scene_node_base_t(){};
    virtual void read_xml(xmlpp::Element* e) = 0;
    virtual void write_xml(xmlpp::Element* e,bool help_comments=false) = 0;
    virtual std::string print(const std::string& prefix="") = 0;
    virtual ~scene_node_base_t(){};
    virtual void prepare(double fs){};
  };

  class object_t : public scene_node_base_t {
  public:
    object_t();
    void read_xml(xmlpp::Element* e);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    std::string name;
    double starttime;
    track_t location;
    euler_track_t orientation;
  };

  class soundfile_t : public scene_node_base_t, public async_sndfile_t {
  public:
    soundfile_t(unsigned int channels);
    void read_xml(xmlpp::Element* e);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    void prepare(double fs);
    std::string filename;
    double gain;
    unsigned int loop;
    double starttime;
    unsigned int firstchannel;
    unsigned int channels;
  };

  class sound_t : public soundfile_t {
  public:
    sound_t();
    void read_xml(xmlpp::Element* e);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    pos_t rel_pos(double t,object_t& parent,object_t& ref);
    pos_t loc;
  };

  class bg_amb_t : public soundfile_t {
  public:
    bg_amb_t();
  };

  class src_object_t : public object_t {
  public:
    src_object_t();
    void read_xml(xmlpp::Element* e);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    std::vector<sound_t> sound;
  };

  class listener_t : public object_t {
  public:
    listener_t();
  };

  class scene_t : public scene_node_base_t {
  public:
    scene_t();
    void read_xml(xmlpp::Element* e);
    void read_xml(const std::string& filename);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    std::string description;
    std::string name;
    double duration;
    double lat;
    double lon;
    double elev;
    std::vector<src_object_t> src;
    std::vector<bg_amb_t> bg_amb;
    listener_t listener;
  };

  scene_t xml_read_scene(const std::string& filename);
  void xml_write_scene(const std::string& filename, scene_t scene, const std::string& comment="", bool help_comments = false);

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
