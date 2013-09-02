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
#include <iostream>
#include "defs.h"
#include "xmlconfig.h"
#include "acousticmodel.h"

namespace TASCAR {

  /** \brief Components relevant for the spatial modelling
   */
  namespace Scene {

    class route_t : public scene_node_base_t {
    public:
      route_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      std::string get_name() const {return name;};
      bool get_mute() const {return mute;};
      bool get_solo() const {return solo;};
      void set_name(const std::string& s) {name=s;};
      void set_mute(bool b) {mute=b;};
      void set_solo(bool b,uint32_t& anysolo);
      bool is_active(uint32_t anysolo);
    private:
      std::string name;
      bool mute;
      bool solo;
    };

    class rgb_color_t {
    public:
      rgb_color_t(double r_,double g_,double b_):r(r_),g(g_),b(b_){};
      rgb_color_t(const std::string& webc);
      rgb_color_t():r(0),g(0),b(0){};
      std::string str();
      double r, g, b;
    };

    class sndfile_info_t : public scene_node_base_t {
    public:
      sndfile_info_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      void prepare(double fs, uint32_t fragsize){};
      std::string fname;
      uint32_t firstchannel;
      uint32_t channels;
      uint32_t objectchannel;
      double starttime;
      uint32_t loopcnt;
      double gain;
      std::string parentname;
    };

    class object_t : public route_t {
    public:
      object_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      bool isactive(double time) const;
      pos_t get_location(double time) const;
      zyx_euler_t get_orientation(double time) const;
      bool is_active(uint32_t anysolo,double t);
      rgb_color_t color;
      double starttime;
      double endtime;
      track_t location;
      euler_track_t orientation;
      pos_t dlocation;
      zyx_euler_t dorientation;
      std::vector<sndfile_info_t> sndfiles;
    };

    class mirror_t {
    public:
      mirror_t():c1(0),c2(0){};
      pos_t p;
      double c1;
      double c2;
    };

    class face_object_t : public object_t, public TASCAR::Acousticmodel::reflector_t {
    public:
      face_object_t();
      ~face_object_t();
      void prepare(double fs, uint32_t fragsize);
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      void geometry_update(double t);
      void process_active(double t,uint32_t anysolo);
      double width;
      double height;
      double reflectivity;
      double damping;
    };

    class src_object_t;

  class connection_t {
  public:
    connection_t(){};
    std::string src;
    std::string dest;
  };

    class jack_port_t {
    public:
      jack_port_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e);
      void set_port_index(uint32_t port_index_);
      uint32_t get_port_index() const { return port_index;};
      void set_portname(const std::string& pn);
      std::string get_portname() const { return portname;};
      std::string get_connect() const { return connect;};
      float get_gain() const { return gain;};
    private:
      std::string portname;
      std::string connect;
      uint32_t port_index;
      float gain;
    };

    class src_diffuse_t : public object_t, public jack_port_t {
    public:
      src_diffuse_t();
      ~src_diffuse_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      void prepare(double fs, uint32_t fragsize);
      void geometry_update(double t);
      void process_active(double t,uint32_t anysolo);
      pos_t size;
      double falloff;
      TASCAR::Acousticmodel::diffuse_source_t* get_source() { return source;};
    private:
      TASCAR::Acousticmodel::diffuse_source_t* source;
    };

    class src_door_t : public object_t, public jack_port_t {
    public:
      src_door_t();
      ~src_door_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      void prepare(double fs, uint32_t fragsize);
      void geometry_update(double t);
      void process_active(double t,uint32_t anysolo);
      double width;
      double height;
      double falloff;
      TASCAR::Acousticmodel::doorsource_t* get_source() { return source;};
    private:
      TASCAR::Acousticmodel::doorsource_t* source;
    };

    class sound_t : public scene_node_base_t, public jack_port_t {
    public:
      sound_t(src_object_t* parent_);
      ~sound_t();
      void set_parent(src_object_t* parent_);
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      void geometry_update(double t);
      void process_active(double t,uint32_t anysolo);
      pos_t get_pos_global(double t) const;
      void prepare(double fs, uint32_t fragsize);
      std::string getlabel();
      bool isactive(double t);
      bool get_mute() const;
      bool get_solo() const;
      std::string get_port_name() const;
      TASCAR::Acousticmodel::pointsource_t* get_source() { return source;};
    private:
      pos_t local_position;
      double chaindist;
      src_object_t* parent;
      std::string name;
      // dynamically allocated source type. Allocated in "prepare",
      // type defined in xml_read:
      TASCAR::Acousticmodel::pointsource_t* source; 
    };

    class src_object_t : public object_t {
    public:
      src_object_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      sound_t* add_sound();
      void prepare(double fs, uint32_t fragsize);
      void geometry_update(double t);
      void process_active(double t,uint32_t anysolo);
      std::vector<sound_t> sound;
    private:
      int32_t startframe;
    };

    class sink_mask_t : public object_t {
    public:
      sink_mask_t();
      void read_xml(xmlpp::Element* e);
      void prepare(double fs, uint32_t fragsize);
      pos_t size;
      double falloff;
    };

    class sink_object_t : public object_t, public jack_port_t {
    public:
      enum sink_type_t {
        omni, cardioid, amb3h3v, amb3h0v, nsp
      };
      sink_object_t();
      ~sink_object_t();
      void read_xml(xmlpp::Element* e);
      void prepare(double fs, uint32_t fragsize);
      TASCAR::Acousticmodel::sink_t* get_sink() { return sink;};
      void geometry_update(double t);
      void process_active(double t,uint32_t anysolo);
      sink_type_t sink_type;
      std::vector<pos_t> spkpos;
      pos_t size;
      double falloff;
      bool render_point;
      bool render_diffuse;
      bool use_mask;
      sink_mask_t mask;
      TASCAR::Acousticmodel::sink_t* sink;
    };

    class range_t : public scene_node_base_t {
    public:
      range_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      void prepare(double fs, uint32_t fragsize){};
      std::string name;
      double start;
      double end;
    };

    class scene_t : public scene_node_base_t {
    public:
      scene_t();
      scene_t(const std::string& filename);
      void read_xml(xmlpp::Element* e);
      void read_xml(const std::string& filename);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      src_object_t* add_source();
      std::vector<sound_t*> linearize_sounds();
      //std::vector<Input::base_t*> linearize_inputs();
      void prepare(double fs, uint32_t fragsize);
      std::string description;
      std::string name;
      double duration;
      void geometry_update(double t);
      void process_active(double t);
      std::vector<src_object_t> object_sources;
      std::vector<src_diffuse_t> diffuse_sources;
      std::vector<src_door_t> door_sources;
      //std::vector<diffuse_reverb_t> reverbs;
      std::vector<face_object_t> faces;
      std::vector<sink_object_t> sink_objects;
      double guiscale;
      //void listener_orientation(zyx_euler_t o){listener.dorientation=o;};
      //void listener_position(pos_t p){listener.dlocation = p;};
      void set_source_position_offset(const std::string& srcname,pos_t position);
      void set_source_orientation_offset(const std::string& srcname,zyx_euler_t position);
      uint32_t anysolo;
      void set_mute(const std::string& name,bool val);
      void set_solo(const std::string& name,bool val);
      bool get_playsound(const sound_t*);
      std::vector<object_t*> get_objects();
      std::vector<range_t> ranges;
      bool loop;
      std::vector<connection_t> connections;
    };

    scene_t xml_read_scene(const std::string& filename);
    void xml_write_scene(const std::string& filename, scene_t scene, const std::string& comment="", bool help_comments = false);

  }

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
