/**
   \file osc_helper.h
   \ingroup libtascar
   \brief helper classes for OSC
   \author Giso Grimm
   \date 2012

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

#ifndef OSC_HELPER_H
#define OSC_HELPER_H

#include <string>
#include <lo/lo.h>

namespace TASCAR {

  class osc_server_t {
  public:
    osc_server_t(const std::string& multicast, const std::string& port,bool verbose=true);
    ~osc_server_t();
    void set_prefix(const std::string& prefix);
    void add_method(const std::string& path,const char* typespec,lo_method_handler h, void *user_data);
    void activate();
    void deactivate();
  private:
    std::string prefix;
    lo_server_thread lost;
    bool isactive;
    bool verbose;
  };

};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

