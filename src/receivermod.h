/**
   \file receivermod.h
   \brief "receivermod" provides base classes for receiver module definitions
   
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
#ifndef RECEIVERMOD_H
#define RECEIVERMOD_H

#include "xmlconfig.h"
#include "audiochunks.h"

namespace TASCAR {

  class receivermod_base_t : public xml_element_t {
  public:
    class data_t {
    public:
      data_t(){};
    };
    receivermod_base_t(xmlpp::Element* xmlsrc);
    virtual void write_xml();
    virtual ~receivermod_base_t();
    virtual void add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*) = 0;
    virtual void add_diffusesource(const pos_t& prel, const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*) = 0;
    virtual void postproc() {};
    virtual uint32_t get_num_channels() = 0;
    virtual std::string get_channel_postfix(uint32_t channel) const { return "";};
    virtual receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize) { return NULL;};
  protected:
  };

  typedef void (*receivermod_error_t)(std::string errmsg);
  typedef TASCAR::receivermod_base_t* (*receivermod_create_t)(xmlpp::Element* xmlsrc,receivermod_error_t errfun);
  typedef void (*receivermod_destroy_t)(TASCAR::receivermod_base_t* h,receivermod_error_t errfun);
  typedef void (*receivermod_write_xml_t)(TASCAR::receivermod_base_t* h,receivermod_error_t errfun);
  typedef void (*receivermod_add_pointsource_t)(TASCAR::receivermod_base_t* h,const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*,receivermod_error_t errfun);
  typedef void (*receivermod_add_diffusesource_t)(TASCAR::receivermod_base_t* h,const pos_t& prel, const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*,receivermod_error_t errfun);
  typedef void (*receivermod_postproc_t)(TASCAR::receivermod_base_t* h,receivermod_error_t errfun);
  typedef uint32_t (*receivermod_get_num_channels_t)(TASCAR::receivermod_base_t* h,receivermod_error_t errfun);
  typedef std::string (*receivermod_get_channel_postfix_t)(TASCAR::receivermod_base_t* h,uint32_t channel,receivermod_error_t errfun);
  typedef receivermod_base_t::data_t* (*receivermod_create_data_t)(TASCAR::receivermod_base_t* h,double srate,uint32_t fragsize,receivermod_error_t errfun);
  
  class receivermod_t : public receivermod_base_t {
  public:
    receivermod_t(xmlpp::Element* xmlsrc);
    void write_xml();
    virtual ~receivermod_t();
    virtual void add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*);
    virtual void add_diffusesource(const pos_t& prel, const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*);
    virtual void postproc();
    virtual uint32_t get_num_channels();
    virtual std::string get_channel_postfix(uint32_t channel);
    virtual receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  private:
    receivermod_t(const receivermod_t&);
    std::string receivertype;
    void* lib;
    TASCAR::receivermod_base_t* libdata;
    receivermod_create_t create_cb;
    receivermod_destroy_t destroy_cb;
    receivermod_write_xml_t write_xml_cb;
    receivermod_add_pointsource_t add_pointsource_cb;
    receivermod_add_diffusesource_t add_diffusesource_cb;
    receivermod_postproc_t postproc_cb;
    receivermod_get_num_channels_t get_num_channels_cb;
    receivermod_get_channel_postfix_t get_channel_postfix_cb;
    receivermod_create_data_t create_data_cb;
  };

}

#define REGISTER_RECEIVERMOD(x)                     \
  extern "C" {                                  \
    TASCAR::receivermod_base_t* receivermod_create(xmlpp::Element* xmlsrc,TASCAR::receivermod_error_t errfun) \
    {                                           \
      try { return new x(xmlsrc); }             \
      catch(const std::exception& e){ errfun(e.what()); return NULL; } \
    }                                           \
    void receivermod_destroy(TASCAR::receivermod_base_t* h,TASCAR::receivermod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ) delete ptr;                     \
      else errfun("Invalid library class pointer (destroy)."); \
    }                                           \
    void receivermod_write_xml(TASCAR::receivermod_base_t* h,TASCAR::receivermod_error_t errfun) \
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
    void receivermod_add_pointsource(TASCAR::receivermod_base_t* h,const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, TASCAR::receivermod_base_t::data_t* data,TASCAR::receivermod_error_t errfun) \
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
    void receivermod_add_diffusesource(TASCAR::receivermod_base_t* h,const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, TASCAR::receivermod_base_t::data_t* data,TASCAR::receivermod_error_t errfun) \
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
    void receivermod_postproc(TASCAR::receivermod_base_t* h,TASCAR::receivermod_error_t errfun) \
    {                                           \
      x*   ptr(dynamic_cast<x*>(h));            \
      if( ptr ){                                \
        try { ptr->postproc(); } \
        catch(const std::exception&e ){         \
          errfun(e.what());                     \
        }                                       \
      }else                                     \
        errfun("Invalid library class pointer (postproc)."); \
    }                                           \
    uint32_t receivermod_get_num_channels(TASCAR::receivermod_base_t* h,TASCAR::receivermod_error_t errfun) \
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
    std::string receivermod_get_channel_postfix(TASCAR::receivermod_base_t* h,uint32_t channel,TASCAR::receivermod_error_t errfun) \
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
    TASCAR::receivermod_base_t::data_t* receivermod_create_data(TASCAR::receivermod_base_t* h,double srate,uint32_t fragsize,TASCAR::receivermod_error_t errfun) \
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
