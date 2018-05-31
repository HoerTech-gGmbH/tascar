/**
   \file sourcemod.h
   \brief "sourcemod" provides base classes for source module definitions
   
   \ingroup libtascar
   \author Giso Grimm
   \date 2017

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
#ifndef SOURCEMOD_H
#define SOURCEMOD_H

#include "tascarplugin.h"
#include "xmlconfig.h"
#include "audiochunks.h"
#include "audiostates.h"

namespace TASCAR {

  class sourcemod_base_t : public xml_element_t, public audiostates_t {
  public:
    class data_t {
    public:
      data_t(){};
      virtual ~data_t(){};
    };
    sourcemod_base_t(xmlpp::Element* xmlsrc);
    virtual ~sourcemod_base_t();
    virtual bool read_source(pos_t& prel, const std::vector<wave_t>& input, wave_t& output, sourcemod_base_t::data_t*) = 0;
    virtual uint32_t get_num_channels() = 0;
    virtual std::string get_channel_postfix(uint32_t channel) const { return "";};
    virtual std::vector<std::string> get_connections() const { return std::vector<std::string>();};
    virtual sourcemod_base_t::data_t* create_data(double srate,uint32_t fragsize) { return NULL;};
  protected:
  };

  class sourcemod_t : public sourcemod_base_t {
  public:
    sourcemod_t(xmlpp::Element* xmlsrc);
    virtual ~sourcemod_t();
    virtual bool read_source( pos_t&, const std::vector<wave_t>&, wave_t&, sourcemod_base_t::data_t* );
    virtual uint32_t get_num_channels();
    virtual std::string get_channel_postfix(uint32_t channel);
    virtual std::vector<std::string> get_connections() const;
    virtual void prepare( chunk_cfg_t& );
    virtual void release();
    virtual sourcemod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  private:
    sourcemod_t(const sourcemod_t&);
    std::string sourcetype;
    void* lib;
    TASCAR::sourcemod_base_t* libdata;
  };

}

#define REGISTER_SOURCEMOD(x) TASCAR_PLUGIN( sourcemod_base_t, xmlpp::Element*, x )


#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

