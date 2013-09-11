/**
  \file osc_helper.cc
  \ingroup libtascar
  \brief helper classes for OSC
  \author  Giso Grimm
  \date 2012
  
  \section license License (LGPL)
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; version 2 of the License.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "osc_helper.h"
#include "errorhandling.h"
#include <iostream>

using namespace TASCAR;

static bool liblo_errflag;

void err_handler(int num, const char *msg, const char *where)
{
  liblo_errflag = true;
  std::cout << "liblo error " << num << ": " << msg << "\n(" << where << ")\n";
}

int osc_set_bool_true(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data )
    *(bool*)(user_data) = true;
  return 0;
}

int osc_set_float(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc == 1) && (types[0] == 'f') )
    *(float*)(user_data) = argv[0]->f;
  return 0;
}

int osc_set_double(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc == 1) && (types[0] == 'f') )
    *(double*)(user_data) = argv[0]->f;
  return 0;
}

int osc_set_int32(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc == 1) && (types[0] == 'i') )
    *(int32_t*)(user_data) = argv[0]->i;
  return 0;
}

osc_server_t::osc_server_t(const std::string& multicast, const std::string& port,bool verbose_)
  : isactive(false),
    verbose(verbose_)
{
  lost = NULL;
  liblo_errflag = false;
  if( multicast.size() ){
    lost = lo_server_thread_new_multicast(multicast.c_str(),port.c_str(),err_handler);
    if( verbose )
      std::cerr << "listening on multicast address \"" << multicast << "\" port " << port << std::endl;
  }else{
    lost = lo_server_thread_new(port.c_str(),err_handler);
    if( verbose )
      std::cerr << "listening on port \"" << port << "\"" << std::endl;
  }
  if( (!lost) || liblo_errflag )
    throw ErrMsg("liblo error (srv_addr: \""+multicast+"\" srv_port: \""+port+"\").");
}

osc_server_t::~osc_server_t()
{
  if( isactive )
    deactivate();
  lo_server_thread_free(lost);
}

void osc_server_t::set_prefix(const std::string& prefix_)
{
  prefix = prefix_;
}

void osc_server_t::add_method(const std::string& path,const char* typespec,lo_method_handler h, void *user_data)
{
  std::string sPath(prefix+path);
  if( verbose )
    std::cerr << "added handler " << sPath << " with typespec \"" << typespec << "\"" << std::endl;
  lo_server_thread_add_method(lost,sPath.c_str(),typespec,h,user_data);
}

void osc_server_t::add_float(const std::string& path,float *data)
{
  add_method(path,"f",osc_set_float,data);
}

void osc_server_t::add_bool_true(const std::string& path,bool *data)
{
  add_method(path,"",osc_set_bool_true,data);
}

void osc_server_t::activate()
{
  lo_server_thread_start(lost);
  isactive = true;
  if( verbose )
    std::cerr << "server active\n";
}

void osc_server_t::deactivate()
{
  lo_server_thread_stop(lost);
  isactive = false;
  if( verbose )
    std::cerr << "server inactive\n";
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

