/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
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
 *
 */

#ifndef PLUGINPROCESSOR_H
#define PLUGINPROCESSOR_H

#include "audioplugin.h"

namespace TASCAR {

  class plugin_processor_t : public audiostates_t, public xml_element_t {
  public:
    plugin_processor_t( xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname );
    ~plugin_processor_t();
    void prepare( chunk_cfg_t& cf_ );
    void release();
    void process_plugins( std::vector<wave_t>& s, const pos_t& p, const zyx_euler_t& o, const transport_t& tp );
    void validate_attributes(std::string& msg) const;
    void add_variables( TASCAR::osc_server_t* srv );
    void add_licenses( licensehandler_t* );
  protected:
    std::vector<TASCAR::audioplugin_t*> plugins;
  };

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
