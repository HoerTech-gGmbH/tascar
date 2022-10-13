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
#ifndef SESSION_READER_H
#define SESSION_READER_H

#include "licensehandler.h"
#include "tscconfig.h"

namespace TASCAR {

  class tsc_reader_t : public TASCAR::xml_doc_t,
                       public licensehandler_t,
                       public licensed_component_t {
  public:
    tsc_reader_t();
    tsc_reader_t(const std::string& filename_or_data, load_type_t t,
                 const std::string& path);
    virtual ~tsc_reader_t();
    void read_xml();
    const std::string& get_session_path() const;
    const std::string& get_file_name() const;

  private:
    tsc_reader_t(const tsc_reader_t&);

  protected:
    virtual void add_scene(tsccfg::node_t){};
    virtual void add_range(tsccfg::node_t){};
    virtual void add_connection(tsccfg::node_t){};
    virtual void add_module(tsccfg::node_t){};
    std::string file_name;
    std::string session_path;
    std::string license;
    std::string attribution;
    bool use_profiler = false;
    std::string profilingpath = "";
  };

} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
