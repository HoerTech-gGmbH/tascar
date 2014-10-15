#ifndef SESSION_H
#define SESSION_H

#include "xmlconfig.h"
#include "scene.h"
#include "audioplayer.h"

namespace TASCAR {

  class session_t;

  class module_base_t : public xml_element_t {
  public:
    module_base_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session_);
    virtual void write_xml();
    virtual ~module_base_t();
  protected:
    TASCAR::session_t* session;
  };

  typedef void (*module_error_t)(std::string errmsg);
  typedef TASCAR::module_base_t* (*module_create_t)(xmlpp::Element* xmlsrc,TASCAR::session_t* session,module_error_t errfun);
  typedef void (*module_destroy_t)(TASCAR::module_base_t* h,module_error_t errfun);
  typedef void (*module_write_xml_t)(TASCAR::module_base_t* h,module_error_t errfun);

  class module_t : public TASCAR::xml_element_t {
  public:
    module_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
    void write_xml();
    virtual ~module_t();
  private:
    std::string name;
    void* lib;
    TASCAR::module_base_t* libdata;
    module_create_t create_cb;
    module_destroy_t destroy_cb;
    module_write_xml_t write_xml_cb;
  };

  class xml_doc_t {
  public:
    xml_doc_t();
    xml_doc_t(const std::string& filename);
    virtual void save(const std::string& filename);
  protected:
    xmlpp::DomParser domp;
    xmlpp::Document* doc;
  };

  class connection_t : public TASCAR::xml_element_t {
  public:
    connection_t(xmlpp::Element*);
    void write_xml();
    std::string src;
    std::string dest;
  };

  class range_t : public scene_node_base_t {
  public:
    range_t(xmlpp::Element* e);
    void write_xml();
    void prepare(double fs, uint32_t fragsize){};
    std::string name;
    double start;
    double end;
  };

  class named_object_t {
  public:
    named_object_t(TASCAR::Scene::object_t* o,const std::string& n) : obj(o),name(n){};
    TASCAR::Scene::object_t* obj;
    std::string name;
  };

  class session_t : public TASCAR::xml_doc_t, public TASCAR::xml_element_t, public jackc_portless_t {
  public:
    session_t();
    session_t(const std::string& filename);
  private:
    session_t(const session_t& src);
  public:
    virtual ~session_t();
    TASCAR::Scene::scene_t* add_scene(xmlpp::Element* e=NULL);
    TASCAR::range_t* add_range(xmlpp::Element* e=NULL);
    TASCAR::connection_t* add_connection(xmlpp::Element* e=NULL);
    TASCAR::module_t* add_module(xmlpp::Element* e=NULL);
    virtual void save(const std::string& filename);
    void write_xml();
    void start();
    void stop();
    void run(bool &b_quit);
    double get_duration() const { return duration; };
    uint32_t get_active_pointsources() const;
    uint32_t get_total_pointsources() const;
    uint32_t get_active_diffusesources() const;
    uint32_t get_total_diffusesources() const;
    std::vector<TASCAR::named_object_t> find_objects(const std::string& pattern);
    //double get_time() const;
    // configuration variables:
    std::string name;
    double duration;
    bool loop;
    std::vector<TASCAR::scene_player_t*> player;
    std::vector<TASCAR::range_t*> ranges;
    std::vector<TASCAR::connection_t*> connections;
    std::vector<TASCAR::module_t*> modules;
    const std::string& get_session_path() const;
  protected:
    // derived variables:
    std::string session_path;
  private:
    void read_xml();
  };

  class actor_module_t : public module_base_t {
  public:
    actor_module_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session,bool fail_on_empty=false);
    void write_xml();
    void set_location(const TASCAR::pos_t& l);
    void set_orientation(const TASCAR::zyx_euler_t& o);
    void add_location(const TASCAR::pos_t& l);
    void add_orientation(const TASCAR::zyx_euler_t& o);
  protected:
    std::string actor;
    std::vector<TASCAR::named_object_t> obj;
  };

}

#define REGISTER_MODULE(x)                                              \
  extern "C" {                                                          \
    TASCAR::module_base_t* tascar_create(xmlpp::Element* xmlsrc,TASCAR::session_t* session,TASCAR::module_error_t errfun) \
    {                                                                   \
      try {                                                             \
        return new x(xmlsrc,session);                                   \
      }                                                                 \
      catch(const std::exception& e){                                   \
        errfun(e.what());                                               \
        return NULL;                                                    \
      }                                                                 \
    }                                                                   \
    void tascar_destroy(TASCAR::module_base_t* h,TASCAR::module_error_t errfun) \
    {                                                                   \
      x* ptr(dynamic_cast<x*>(h));                                      \
      if( ptr )                                                         \
        delete ptr;                                                     \
      else                                                              \
        errfun("Invalid library class pointer (destroy).");             \
    }                                                                   \
    void tascar_write_xml(TASCAR::module_base_t* h,TASCAR::module_error_t errfun) \
    {                                                                   \
      x* ptr(dynamic_cast<x*>(h));                                      \
      if( ptr ){                                                        \
        try {                                                           \
          ptr->write_xml();                                             \
        }                                                               \
        catch(const std::exception&e ){                                 \
          errfun(e.what());                                             \
        }                                                               \
      }else                                                             \
        errfun("Invalid library class pointer (write_xml).");           \
    }                                                                   \
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
