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

#include <vector>
#include <string>
#include <lo/lo.h>

namespace TASCAR {

  class osc_server_t {
  public:
    class descriptor_t {
    public:
      std::string path;
      std::string typespec;
    };
    osc_server_t(const std::string& multicast, const std::string& port,bool verbose=true);
    ~osc_server_t();
    void set_prefix(const std::string& prefix);
    const std::string& get_prefix() const;
    void add_method(const std::string& path,const char* typespec,lo_method_handler h, void *user_data);
    void add_double(const std::string& path,double *data);
    void add_double_db(const std::string& path,double *data);
    void add_double_dbspl(const std::string& path,double *data);
    void add_double_degree(const std::string& path,double *data);
    void add_float(const std::string& path,float *data);
    void add_float_db(const std::string& path,float *data);
    void add_float_degree(const std::string& path,float *data);
    void add_vector_float(const std::string& path,std::vector<float> *data);
    void add_bool_true(const std::string& path,bool *data);
    void add_bool_false(const std::string& path,bool *data);
    void add_bool(const std::string& path,bool *data);
    void add_int(const std::string& path,int32_t *data);
    void add_uint(const std::string& path,uint32_t *data);
    void activate();
    void deactivate();
    std::string list_variables() const;
    std::vector<descriptor_t> variables;
    const std::string osc_srv_addr;
    const std::string osc_srv_port;
  private:
    std::string prefix;
    lo_server_thread lost;
    bool initialized;
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

