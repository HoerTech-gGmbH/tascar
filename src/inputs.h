#ifndef INPUTS_H
#define INPUTS_H

#include "scene.h"

namespace TASCAR {

  namespace Input {

    class base_t : public TASCAR::scene_node_base_t  {
    public:
      base_t();
      virtual ~base_t();
      virtual void fill(int32_t tp_firstframe, bool tp_running) = 0;
      virtual void prepare(double fs, uint32_t fragsize);
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      float* get_buffer(uint32_t n);
      virtual std::string get_tag() = 0;
      std::string name;
    protected:
      uint32_t size;
      float* data;
    };
  
    class file_t : public TASCAR::Input::base_t, public TASCAR::async_sndfile_t {
    public:
      file_t();
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      std::string print(const std::string& prefix="");
      void prepare(double fs, uint32_t fragsize);
      void fill(int32_t tp_firstframe, bool tp_running);
      std::string get_tag() { return "file";};
      std::string filename;
      double gain;
      unsigned int loop;
      double starttime;
      unsigned int firstchannel;
    };

    class jack_t : public TASCAR::Input::base_t {
    public:
      jack_t(const std::string& parent_name_);
      void fill(int32_t tp_firstframe, bool tp_running){};
      std::string get_tag() { return "jack";};
      void write(uint32_t n,float* b);
      void read_xml(xmlpp::Element* e);
      void write_xml(xmlpp::Element* e,bool help_comments=false);
      std::string print(const std::string& prefix="");
      std::string get_port_name() const { return parent_name+"."+name;};
      std::vector<std::string> connections;
      std::string parent_name;
      double gain;
    };

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
