#ifndef AUDIOPLUGIN_H
#define AUDIOPLUGIN_H

#include "xmlconfig.h"
#include "audiochunks.h"

namespace TASCAR {

  class audioplugin_base_t : public xml_element_t {
  public:
    audioplugin_base_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
    virtual void write_xml() {};
    virtual ~audioplugin_base_t();
    virtual void ap_process(wave_t& chunk, const TASCAR::pos_t& pos, double t, bool tp_rolling) = 0;
    virtual void prepare(double srate,uint32_t fragsize) {};
    void prepare_(double srate,uint32_t fragsize);
    virtual void release() {};
    void release_();
    virtual void add_variables( TASCAR::osc_server_t* srv ) {};
    const std::string& get_name() const { return name; };
  protected:
    double f_sample;
    double f_fragment;
    double t_sample;
    double t_fragment;
    uint32_t n_fragment;
    std::string name;
  private:
    bool prepared;
  };

  typedef void (*audioplugin_error_t)(std::string errmsg);
  typedef TASCAR::audioplugin_base_t* (*audioplugin_create_t)(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname, audioplugin_error_t errfun);
  typedef void (*audioplugin_destroy_t)(TASCAR::audioplugin_base_t* h,audioplugin_error_t errfun);
  typedef void (*audioplugin_write_xml_t)(TASCAR::audioplugin_base_t* h,audioplugin_error_t errfun);
  typedef void (*audioplugin_process_t)(TASCAR::audioplugin_base_t* h, wave_t& chunk, const TASCAR::pos_t& pos, double t, bool tp_rolling, audioplugin_error_t errfun);
  typedef void (*audioplugin_prepare_t)(TASCAR::audioplugin_base_t* h, double srate, uint32_t fragsize, audioplugin_error_t errfun);
  typedef void (*audioplugin_release_t)(TASCAR::audioplugin_base_t* h, audioplugin_error_t errfun);
  typedef void (*audioplugin_add_variables_t)(TASCAR::audioplugin_base_t* h, TASCAR::osc_server_t* srv, audioplugin_error_t errfun);

  class audioplugin_t : public audioplugin_base_t {
  public:
    audioplugin_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
    void write_xml();
    virtual ~audioplugin_t();
    virtual void ap_process(wave_t& chunk, const TASCAR::pos_t& pos, double t, bool tp_rolling);
    virtual void prepare(double srate,uint32_t fragsize);
    virtual void add_variables( TASCAR::osc_server_t* srv );
    virtual void release();
  private:
    audioplugin_t(const audioplugin_t&);
    std::string plugintype;
    void* lib;
    TASCAR::audioplugin_base_t* libdata;
    audioplugin_create_t create_cb;
    audioplugin_destroy_t destroy_cb;
    audioplugin_write_xml_t write_xml_cb;
    audioplugin_process_t process_cb;
    audioplugin_prepare_t prepare_cb;
    audioplugin_release_t release_cb;
    audioplugin_add_variables_t add_variables_cb;
  };

}
#define REGISTER_AUDIOPLUGIN(x)                     \
  extern "C" {                                  \
    TASCAR::audioplugin_base_t* audioplugin_create(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname, TASCAR::audioplugin_error_t errfun) \
    {                                           \
      try { return new x(xmlsrc,name,parentname); }                              \
      catch(const std::exception& e){ errfun(e.what()); return NULL; } \
    }                                           \
    void audioplugin_destroy(TASCAR::audioplugin_base_t* h,TASCAR::audioplugin_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ) delete ptr;                     \
      else errfun("Invalid library class pointer (destroy)."); \
    }                                           \
    void audioplugin_write_xml(TASCAR::audioplugin_base_t* h,TASCAR::audioplugin_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { ptr->write_xml(); }               \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (write_xml)."); \
    }                                           \
    void audioplugin_process(TASCAR::audioplugin_base_t* h, TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, double t, bool tp_rolling, TASCAR::audioplugin_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { ptr->ap_process(chunk, pos, t, tp_rolling ); }        \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (process)."); \
    }                                           \
    void audioplugin_prepare(TASCAR::audioplugin_base_t* h,double srate,uint32_t fragsize,TASCAR::audioplugin_error_t errfun) \
    {                                           \
      if( h ){                                \
        try { return h->prepare_(srate,fragsize); }      \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (prepare)."); \
    }                                           \
    void audioplugin_release(TASCAR::audioplugin_base_t* h, TASCAR::audioplugin_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { return ptr->release_(); }      \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (release)."); \
    }                                           \
    void audioplugin_add_variables(TASCAR::audioplugin_base_t* h, TASCAR::osc_server_t* srv, TASCAR::audioplugin_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { return ptr->add_variables(srv); }      \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (add_variables)."); \
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
