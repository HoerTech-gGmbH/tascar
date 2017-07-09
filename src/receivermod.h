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

#include "speakerarray.h"
#include "tascarplugin.h"

namespace TASCAR {

  class receivermod_base_t : public xml_element_t {
  public:
    class data_t {
    public:
      data_t(){};
      virtual ~data_t(){};
    };
    receivermod_base_t(xmlpp::Element* xmlsrc);
    virtual void write_xml();
    virtual ~receivermod_base_t();
    virtual void add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*) = 0;
    virtual void add_diffusesource(const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*) = 0;
    virtual void postproc(std::vector<wave_t>& output) {};
    virtual uint32_t get_num_channels() = 0;
    virtual std::string get_channel_postfix(uint32_t channel) const { return "";};
    virtual std::vector<std::string> get_connections() const { return std::vector<std::string>();};
    virtual void configure(double srate,uint32_t fragsize) {};
    virtual receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize) { return NULL;};
  protected:
  };

  class receivermod_base_speaker_t : public receivermod_base_t {
  public:
    receivermod_base_speaker_t(xmlpp::Element* xmlsrc);
    virtual void write_xml();
    virtual std::vector<std::string> get_connections() const;
    virtual void postproc(std::vector<wave_t>& output);
    virtual void configure(double srate,uint32_t fragsize);
  protected:
    TASCAR::spk_array_t spkpos;
  };

  class receivermod_t : public receivermod_base_t {
  public:
    receivermod_t(xmlpp::Element* xmlsrc);
    void write_xml();
    virtual ~receivermod_t();
    virtual void add_pointsource(const pos_t& prel, const wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*);
    virtual void add_diffusesource(const amb1wave_t& chunk, std::vector<wave_t>& output, receivermod_base_t::data_t*);
    virtual void postproc(std::vector<wave_t>& output);
    virtual uint32_t get_num_channels();
    virtual std::string get_channel_postfix(uint32_t channel);
    virtual std::vector<std::string> get_connections() const;
    virtual void configure(double srate,uint32_t fragsize);
    virtual receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  private:
    receivermod_t(const receivermod_t&);
    std::string receivertype;
    void* lib;
    TASCAR::receivermod_base_t* libdata;
  };

}

#define REGISTER_RECEIVERMOD(x) TASCAR_PLUGIN( receivermod_base_t, xmlpp::Element*, x )

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
