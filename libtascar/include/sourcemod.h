/**
 * @file sourcemod.h
 * @brief Source module base class for source directivity
 * @ingroup libtascar
 * @author Giso Grimm
 * @date 2017
 */
/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef SOURCEMOD_H
#define SOURCEMOD_H

#include "audiochunks.h"
#include "audiostates.h"
#include "tascarplugin.h"
#include "tscconfig.h"

namespace TASCAR {

  class sourcemod_base_t : public xml_element_t, public audiostates_t {
  public:
    class data_t {
    public:
      data_t(){};
      virtual ~data_t(){};
    };
    sourcemod_base_t(tsccfg::node_t xmlsrc);
    virtual ~sourcemod_base_t();
    virtual bool read_source(pos_t& prel, const std::vector<wave_t>& input, wave_t& output, sourcemod_base_t::data_t*);
    virtual bool read_source_diffuse(pos_t& prel, const std::vector<wave_t>& input, wave_t& output, sourcemod_base_t::data_t*);
    virtual std::vector<std::string> get_connections() const { return std::vector<std::string>();};
    virtual sourcemod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const = 0;
    virtual void configure();
  protected:
  };

  class sourcemod_t : public sourcemod_base_t {
  public:
    sourcemod_t(tsccfg::node_t xmlsrc);
    virtual ~sourcemod_t();
    virtual bool read_source( pos_t&, const std::vector<wave_t>&, wave_t&, sourcemod_base_t::data_t* );
    virtual bool read_source_diffuse( pos_t&, const std::vector<wave_t>&, wave_t&, sourcemod_base_t::data_t* );
    virtual std::vector<std::string> get_connections() const;
    virtual void configure();
    virtual void release();
    void post_prepare();
    virtual sourcemod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  private:
    sourcemod_t(const sourcemod_t&);
    std::string sourcetype;
    void* lib;
    TASCAR::sourcemod_base_t* libdata;
  };

}

#define REGISTER_SOURCEMOD(x) TASCAR_PLUGIN( sourcemod_base_t, tsccfg::node_t, x )


#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

