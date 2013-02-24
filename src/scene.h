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
#include <iostream>
#include "defs.h"

namespace TASCAR {

  class scene_node_base_t {
  public:
    scene_node_base_t(){};
    virtual void read_xml(xmlpp::Element* e) = 0;
    virtual void write_xml(xmlpp::Element* e,bool help_comments=false) = 0;
    virtual std::string print(const std::string& prefix="") = 0;
    virtual ~scene_node_base_t(){};
    virtual void prepare(double fs) = 0;
  };

  class rgb_color_t {
  public:
    rgb_color_t(double r_,double g_,double b_):r(r_),g(g_),b(b_){};
    rgb_color_t(const std::string& webc);
    rgb_color_t():r(0),g(0),b(0){};
    std::string str();
    double r, g, b;
  };
  
  class object_t : public scene_node_base_t {
  public:
    object_t();
    void read_xml(xmlpp::Element* e);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    bool isactive(double time) const;
    std::string name;
    rgb_color_t color;
    double starttime;
    double endtime;
    bool muted;
    track_t location;
    euler_track_t orientation;
    pos_t dlocation;
    zyx_euler_t dorientation;
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
    sound_t(object_t* parent_,object_t* reference_);
    void request_data( int32_t firstframe, uint32_t n, uint32_t channels, float** buf );
    void set_reference(object_t* reference_);
    void set_parent(object_t* parent_);
    void read_xml(xmlpp::Element* e);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    pos_t get_pos(double t) const;
    pos_t get_pos_global(double t) const;
    void prepare(double fs);
    std::string getlabel();
    bool isactive(double t){ return parent && parent->isactive(t);};
  private:
    pos_t loc;
    // std::vector<pos_t> vertices;
    double chaindist;
    object_t* parent;
    object_t* reference;
    double fs_;
  };

  class bg_amb_t : public soundfile_t {
  public:
    bg_amb_t();
  };

  class src_object_t : public object_t {
  public:
    src_object_t(object_t* reference);
    void set_reference(object_t* reference_);
    void read_xml(xmlpp::Element* e);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    sound_t* add_sound();
    void prepare(double fs);
    std::vector<sound_t> sound;
  private:
    object_t* reference;
  };

  class listener_t : public object_t {
  public:
    listener_t();
    void prepare(double fs){};
  };

  class scene_t : public scene_node_base_t {
  public:
    scene_t();
    void read_xml(xmlpp::Element* e);
    void read_xml(const std::string& filename);
    void write_xml(xmlpp::Element* e,bool help_comments=false);
    std::string print(const std::string& prefix="");
    src_object_t* add_source();
    std::vector<sound_t*> linearize();
    void prepare(double fs);
    std::string description;
    std::string name;
    double duration;
    double lat;
    double lon;
    double elev;
    std::vector<src_object_t> src;
    std::vector<bg_amb_t> bg_amb;
    listener_t listener;
    double guiscale;
    void listener_orientation(zyx_euler_t o){listener.dorientation=o;};
    void listener_position(pos_t p){listener.dlocation = p;};
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
