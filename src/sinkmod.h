/**
   \file sinkmod.h
   \brief "sinkmod" provides base classes for sink module definitions
   
   \ingroup libtascar
   \author Giso Grimm
   \date 2014

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
#ifndef SINKMOD_H
#define SINKMOD_H

#include "xmlconfig.h"
#include "audiochunks.h"

namespace TASCAR {

  class sinkmod_base_t : public xml_element_t {
  public:
    class data_t {
    public:
      data_t(){};
    };
    sinkmod_base_t(xmlpp::Element* xmlsrc);
    virtual void write_xml();
    virtual ~sinkmod_base_t();
    virtual void add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t*) = 0;
    virtual void add_diffusesource(const pos_t& prel, const amb1wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t*) = 0;
    virtual uint32_t get_num_channels() = 0;
    virtual std::string get_channel_postfix(uint32_t channel) const { return "";};
    virtual sinkmod_base_t::data_t* create_data(double srate,uint32_t fragsize) { return NULL;};
  protected:
  };

  typedef void (*sinkmod_error_t)(std::string errmsg);
  typedef TASCAR::sinkmod_base_t* (*sinkmod_create_t)(xmlpp::Element* xmlsrc,sinkmod_error_t errfun);
  typedef void (*sinkmod_destroy_t)(TASCAR::sinkmod_base_t* h,sinkmod_error_t errfun);
  typedef void (*sinkmod_write_xml_t)(TASCAR::sinkmod_base_t* h,sinkmod_error_t errfun);
  typedef void (*sinkmod_add_pointsource_t)(TASCAR::sinkmod_base_t* h,const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t*,sinkmod_error_t errfun);
  typedef void (*sinkmod_add_diffusesource_t)(TASCAR::sinkmod_base_t* h,const pos_t& prel, const amb1wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t*,sinkmod_error_t errfun);
  typedef uint32_t (*sinkmod_get_num_channels_t)(TASCAR::sinkmod_base_t* h,sinkmod_error_t errfun);
  typedef std::string (*sinkmod_get_channel_postfix_t)(TASCAR::sinkmod_base_t* h,uint32_t channel,sinkmod_error_t errfun);
  typedef sinkmod_base_t::data_t* (*sinkmod_create_data_t)(TASCAR::sinkmod_base_t* h,double srate,uint32_t fragsize,sinkmod_error_t errfun);
  
  class sinkmod_t : public TASCAR::xml_element_t {
  public:
    sinkmod_t(xmlpp::Element* xmlsrc);
    void write_xml();
    virtual ~sinkmod_t();
    virtual void add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t*);
    virtual void add_diffusesource(const pos_t& prel, const amb1wave_t& chunk, std::vector<wave_t>& output, sinkmod_base_t::data_t*);
    virtual uint32_t get_num_channels();
    virtual std::string get_channel_postfix(uint32_t channel);
    virtual sinkmod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  private:
    sinkmod_t(const sinkmod_t&);
    std::string sinktype;
    void* lib;
    TASCAR::sinkmod_base_t* libdata;
    sinkmod_create_t create_cb;
    sinkmod_destroy_t destroy_cb;
    sinkmod_write_xml_t write_xml_cb;
    sinkmod_add_pointsource_t add_pointsource_cb;
    sinkmod_add_diffusesource_t add_diffusesource_cb;
    sinkmod_get_num_channels_t get_num_channels_cb;
    sinkmod_get_channel_postfix_t get_channel_postfix_cb;
    sinkmod_create_data_t create_data_cb;
  };

}

#define REGISTER_SINKMOD(x)                     \
  extern "C" {                                  \
    TASCAR::sinkmod_base_t* sinkmod_create(xmlpp::Element* xmlsrc,TASCAR::sinkmod_error_t errfun) \
    {                                           \
      try { return new x(xmlsrc); }             \
      catch(const std::exception& e){ errfun(e.what()); return NULL; } \
    }                                           \
    void sinkmod_destroy(TASCAR::sinkmod_base_t* h,TASCAR::sinkmod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ) delete ptr;                     \
      else errfun("Invalid library class pointer (destroy)."); \
    }                                           \
    void sinkmod_write_xml(TASCAR::sinkmod_base_t* h,TASCAR::sinkmod_error_t errfun) \
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
    void sinkmod_add_pointsource(TASCAR::sinkmod_base_t* h,const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, TASCAR::sinkmod_base_t::data_t* data,TASCAR::sinkmod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { ptr->add_pointsource(prel,chunk,output,data); } \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (add_pointsource)."); \
    }                                           \
    void sinkmod_add_diffusesource(TASCAR::sinkmod_base_t* h,const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, TASCAR::sinkmod_base_t::data_t* data,TASCAR::sinkmod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { ptr->add_diffusesource(prel,chunk,output,data); } \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (add_diffusesource)."); \
    }                                           \
    uint32_t sinkmod_get_num_channels(TASCAR::sinkmod_base_t* h,TASCAR::sinkmod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { return ptr->get_num_channels(); } \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (get_num_channels)."); \
      return 0;                                 \
    }                                           \
    std::string sinkmod_get_channel_postfix(TASCAR::sinkmod_base_t* h,uint32_t channel,TASCAR::sinkmod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { return ptr->get_channel_postfix(channel); } \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (get_num_channel_postfix)."); \
      return "";                                \
    }                                           \
    TASCAR::sinkmod_base_t::data_t* sinkmod_create_data(TASCAR::sinkmod_base_t* h,double srate,uint32_t fragsize,TASCAR::sinkmod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { return ptr->create_data(srate,fragsize); }      \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (create_data)."); \
      return NULL;                                \
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
