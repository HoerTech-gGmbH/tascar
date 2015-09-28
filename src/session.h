#ifndef SESSION_H
#define SESSION_H

#include "audioplayer.h"

namespace TASCAR {

  class session_t;

  class module_base_t : public xml_element_t {
  public:
    module_base_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session_);
    virtual void write_xml();
    virtual ~module_base_t();
    /**
       \brief Method to update geometry etc on each processing cycle in the session processing thread (jack).
       \param t Transport time.
       \param running Transport running (true) or stopped (false).
     */
    virtual void update(uint32_t frame,bool running);
    virtual void configure(double srate,uint32_t fragsize);
  protected:
    TASCAR::session_t* session;
  };

  typedef void (*module_error_t)(std::string errmsg);
  typedef TASCAR::module_base_t* (*module_create_t)(xmlpp::Element* xmlsrc,TASCAR::session_t* session,module_error_t errfun);
  typedef void (*module_destroy_t)(TASCAR::module_base_t* h,module_error_t errfun);
  typedef void (*module_write_xml_t)(TASCAR::module_base_t* h,module_error_t errfun);
  typedef void (*module_update_t)(TASCAR::module_base_t* h,module_error_t errfun,uint32_t frame,bool running);
  typedef void (*module_configure_t)(TASCAR::module_base_t* h,module_error_t errfun,double srate,uint32_t fragsize);

  class module_t : public TASCAR::xml_element_t {
  public:
    module_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
    void write_xml();
    virtual ~module_t();
    void configure(double srate,uint32_t fragsize);
    void update(uint32_t frame,bool running);
  private:
    std::string name;
    void* lib;
    TASCAR::module_base_t* libdata;
    module_create_t create_cb;
    module_destroy_t destroy_cb;
    module_write_xml_t write_xml_cb;
    module_update_t update_cb;
    module_configure_t configure_cb;
  };

  class xml_doc_t {
  public:
    enum load_type_t { LOAD_FILE, LOAD_STRING };
    xml_doc_t();
    xml_doc_t(const std::string& filename,load_type_t t);
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

  class session_t : public TASCAR::xml_doc_t, public TASCAR::xml_element_t, public jackc_transport_t, public TASCAR::osc_server_t {
  public:
    session_t();
    session_t(const std::string& filename_or_data,load_type_t t,const std::string& path);
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
    std::vector<std::string> get_render_output_ports() const;
    virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running);
    //virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
    void unload_modules();
  protected:
    // derived variables:
    std::string session_path;
  private:
    void add_transport_methods();
    void read_xml();
    double period_time;
    bool started_;
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

#define REGISTER_MODULE_UPDATE(x)               \
  extern "C" {                                  \
    void tascar_update(TASCAR::module_base_t* h,TASCAR::module_error_t errfun,uint32_t frame,bool running) \
    {                                           \
      x* ptr(dynamic_cast<x*>(h));              \
      if( ptr ){                                \
        try {                                   \
          ptr->update(frame,running);               \
        }                                       \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (update)."); \
    }                                           \
    void tascar_configure(TASCAR::module_base_t* h,TASCAR::module_error_t errfun,double srate,uint32_t fragsize) \
    {                                           \
      x* ptr(dynamic_cast<x*>(h));              \
      if( ptr ){                                \
        try {                                   \
          ptr->configure(srate,fragsize);        \
        }                                       \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (configure)."); \
    }                                           \
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
