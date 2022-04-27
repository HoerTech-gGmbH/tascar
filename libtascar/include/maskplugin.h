/**
 * @file   audioplugin.h
 * @author Giso Grimm
 *
 * @brief  Base class for audio signal processing plugins
 */
/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
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
#ifndef MASKPLUGIN_H
#define MASKPLUGIN_H

#include "audiochunks.h"
#include "audiostates.h"
#include "licensehandler.h"
#include "osc_helper.h"
#include "tascarplugin.h"

namespace TASCAR {

  class maskplugin_cfg_t {
  public:
    maskplugin_cfg_t(tsccfg::node_t xmlsrc) : xmlsrc(xmlsrc){};
    tsccfg::node_t xmlsrc;
    std::string modname;
  };

  /**
     \brief Base class of audio processing plugins
  */
  class maskplugin_base_t : public xml_element_t,
                            public audiostates_t,
                            public licensed_component_t {
  public:
    maskplugin_base_t(const maskplugin_cfg_t& cfg);
    virtual ~maskplugin_base_t();
    virtual float get_gain(const TASCAR::pos_t& pos) = 0;
    virtual void get_diff_gain(float* gm) = 0;
    virtual void add_variables(TASCAR::osc_server_t*){};
    const std::string& get_modname() const { return modname; };
    float drawradius = 0.0f;

  protected:
    std::string modname;
  };

  class maskplugin_t : public maskplugin_base_t {
  public:
    maskplugin_t(const maskplugin_cfg_t& cfg);
    virtual ~maskplugin_t();
    virtual float get_gain(const TASCAR::pos_t& pos);
    virtual void get_diff_gain(float* gm);
    virtual void add_variables(TASCAR::osc_server_t* srv);
    virtual void add_licenses(licensehandler_t* srv);
    virtual void validate_attributes(std::string&) const;

  private:
    maskplugin_t(const maskplugin_t&);
    std::string plugintype;
    void* lib;
    TASCAR::maskplugin_base_t* libdata;
  };

} // namespace TASCAR

#define REGISTER_MASKPLUGIN(x)                                                 \
  TASCAR_PLUGIN(maskplugin_base_t, const maskplugin_cfg_t&, x)

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
