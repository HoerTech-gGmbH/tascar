/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/* License (GPL)
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

#include "osc_helper.h"
#include "defs.h"
#include "errorhandling.h"
#include <map>
#include <math.h>

using namespace TASCAR;

static bool liblo_errflag;

std::string str_get_null(void* data)
{
  return "null";
}

TASCAR::osc_server_t::data_element_t::data_element_t()
    : ptr(NULL), cnv(str_get_null)
{
}

TASCAR::osc_server_t::data_element_t::data_element_t(const std::string& p,
                                                     void* ptr_,
                                                     strcnvrt_t* cnv_,
                                                     const std::string& type_)
    : ptr(ptr_), cnv(cnv_), path(p), type(type_)
{
  size_t pos = path.rfind("/");
  if(pos != std::string::npos) {
    name = path.substr(pos + 1);
    prefix = path.substr(0, pos);
  } else {
    prefix = "";
    name = path;
  }
}

void err_handler(int num, const char* msg, const char* where)
{
  liblo_errflag = true;
  std::cout << "liblo error " << num << ": " << msg << "\n(" << where << ")\n";
}

int osc_set_bool_true(const char* path, const char* types, lo_arg** argv,
                      int argc, lo_message msg, void* user_data)
{
  if(user_data)
    *(bool*)(user_data) = true;
  return 0;
}

int osc_set_bool_false(const char* path, const char* types, lo_arg** argv,
                       int argc, lo_message msg, void* user_data)
{
  if(user_data)
    *(bool*)(user_data) = false;
  return 0;
}

int osc_set_float(const char* path, const char* types, lo_arg** argv, int argc,
                  lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(float*)(user_data) = argv[0]->f;
  return 0;
}

std::string str_get_float(void* data)
{
  return TASCAR::to_string(*(float*)(data));
}

int osc_get_float(const char* path, const char* types, lo_arg** argv, int argc,
                  lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(), *(float*)(user_data));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_float_db(const char* path, const char* types, lo_arg** argv,
                     int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(float*)(user_data) = powf(10.0f, 0.05 * argv[0]->f);
  return 0;
}

int osc_get_float_db(const char* path, const char* types, lo_arg** argv,
                     int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(),
              20.0f * log10f(*(float*)(user_data)));
      lo_address_free(target);
    }
  }
  return 0;
}

std::string str_get_float_db(void* data)
{
  return TASCAR::to_string(20.0f * log10f(*(float*)(data)));
}

int osc_set_float_dbspl(const char* path, const char* types, lo_arg** argv,
                        int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(float*)(user_data) = powf(10.0f, 0.05 * argv[0]->f) * 2e-5f;
  return 0;
}

std::string str_get_float_dbspl(void* data)
{
  return TASCAR::to_string(20.0f * log10f(*(float*)(data)*5.0e4f));
}

int osc_get_float_dbspl(const char* path, const char* types, lo_arg** argv,
                        int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(),
              20.0f * log10f(*(float*)(user_data)*5.0e4f));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_vector_float(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data)
{
  if(user_data) {
    std::vector<float>* data((std::vector<float>*)user_data);
    if(argc == (int)(data->size()))
      for(int k = 0; k < argc; ++k)
        (*data)[k] = argv[k]->f;
  }
  return 0;
}

int osc_set_vector_float_dbspl(const char* path, const char* types,
                               lo_arg** argv, int argc, lo_message msg,
                               void* user_data)
{
  if(user_data) {
    std::vector<float>* data((std::vector<float>*)user_data);
    if(argc == (int)(data->size()))
      for(int k = 0; k < argc; ++k)
        (*data)[k] = powf(10.0f, 0.05f * argv[k]->f) * 2e-5f;
  }
  return 0;
}

int osc_set_vector_float_db(const char* path, const char* types, lo_arg** argv,
                            int argc, lo_message msg, void* user_data)
{
  if(user_data) {
    std::vector<float>* data((std::vector<float>*)user_data);
    if(argc == (int)(data->size()))
      for(int k = 0; k < argc; ++k)
        (*data)[k] = powf(10.0f, 0.05f * argv[k]->f);
  }
  return 0;
}

int osc_set_vector_double(const char* path, const char* types, lo_arg** argv,
                          int argc, lo_message msg, void* user_data)
{
  if(user_data) {
    std::vector<double>* data((std::vector<double>*)user_data);
    if(argc == (int)(data->size()))
      for(int k = 0; k < argc; ++k)
        (*data)[k] = argv[k]->f;
  }
  return 0;
}

int osc_set_double_db(const char* path, const char* types, lo_arg** argv,
                      int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(double*)(user_data) = pow(10.0, 0.05 * argv[0]->f);
  return 0;
}

std::string str_get_double_db(void* data)
{
  return TASCAR::to_string(20.0 * log10(*(double*)(data)));
}

int osc_get_double_db(const char* path, const char* types, lo_arg** argv,
                      int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(),
              20.0f * log10f(*(double*)(user_data)));
      lo_address_free(target);
    }
  }
  return 0;
}

std::string str_get_double_dbspl(void* data)
{
  return TASCAR::to_string(20.0 * log10(*(double*)(data)*5e4));
}

int osc_set_double_dbspl(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(double*)(user_data) = pow(10.0, 0.05 * argv[0]->f) * 2e-5;
  return 0;
}

int osc_get_double_dbspl(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(),
              20.0f * log10f(*(double*)(user_data)*5.0e4f));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_float_degree(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(float*)(user_data) = DEG2RADf * argv[0]->f;
  return 0;
}

std::string str_get_float_degree(void* data)
{
  return TASCAR::to_string(RAD2DEGf * *(float*)(data));
}

int osc_get_float_degree(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(),
              RAD2DEGf * *(float*)(user_data));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_double_degree(const char* path, const char* types, lo_arg** argv,
                          int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(double*)(user_data) = DEG2RAD * argv[0]->f;
  return 0;
}

std::string str_get_double_degree(void* data)
{
  return TASCAR::to_string(RAD2DEG * *(double*)(data));
}

int osc_get_double_degree(const char* path, const char* types, lo_arg** argv,
                          int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(),
              RAD2DEGf * *(double*)(user_data));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_double(const char* path, const char* types, lo_arg** argv, int argc,
                   lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    *(double*)(user_data) = argv[0]->f;
  return 0;
}

std::string str_get_double(void* data)
{
  return TASCAR::to_string(*(double*)(data));
}

int osc_get_double(const char* path, const char* types, lo_arg** argv, int argc,
                   lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "sf", npath.c_str(),
              *(double*)(user_data));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_int32(const char* path, const char* types, lo_arg** argv, int argc,
                  lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'i'))
    *(int32_t*)(user_data) = argv[0]->i;
  return 0;
}

int osc_get_int32(const char* path, const char* types, lo_arg** argv, int argc,
                  lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "si", npath.c_str(),
              *(int32_t*)(user_data));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_uint32(const char* path, const char* types, lo_arg** argv, int argc,
                   lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'i'))
    *(uint32_t*)(user_data) = (uint32_t)(argv[0]->i);
  return 0;
}

std::string str_get_uint(void* data)
{
  return TASCAR::to_string(*(uint32_t*)(data));
}

std::string str_get_int(void* data)
{
  return TASCAR::to_string(*(int32_t*)(data));
}

std::string str_get_string(void* data)
{
  return *(std::string*)(data);
}

int osc_get_uint32(const char* path, const char* types, lo_arg** argv, int argc,
                   lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "si", npath.c_str(),
              *(uint32_t*)(user_data));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_bool(const char* path, const char* types, lo_arg** argv, int argc,
                 lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'i'))
    *(bool*)(user_data) = (argv[0]->i != 0);
  return 0;
}

std::string str_get_bool(void* data)
{
  return TASCAR::to_string_bool(*(bool*)(data));
}

int osc_get_bool(const char* path, const char* types, lo_arg** argv, int argc,
                 lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "si", npath.c_str(), *(bool*)(user_data));
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_set_string(const char* path, const char* types, lo_arg** argv, int argc,
                   lo_message msg, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 's'))
    *(std::string*)(user_data) = &(argv[0]->s);
  return 0;
}

int osc_get_string(const char* path, const char* types, lo_arg** argv, int argc,
                   lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      std::string npath(path);
      if(npath.size() > 4)
        npath = npath.substr(0, npath.size() - 4);
      lo_send(target, &(argv[1]->s), "ss", npath.c_str(),
              ((std::string*)(user_data))->c_str());
      lo_address_free(target);
    }
  }
  return 0;
}

int osc_send_variables(const char* path, const char* types, lo_arg** argv,
                       int argc, lo_message msg, void* user_data)
{
  if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
    osc_server_t* srv(reinterpret_cast<osc_server_t*>(user_data));
    srv->send_variable_list(&(argv[0]->s), &(argv[1]->s));
  }
  if(user_data && (argc == 3) && (types[0] == 's') && (types[1] == 's') &&
     (types[2] == 's')) {
    osc_server_t* srv(reinterpret_cast<osc_server_t*>(user_data));
    srv->send_variable_list(&(argv[0]->s), &(argv[1]->s), &(argv[2]->s));
  }
  return 0;
}

int string2proto(const std::string& proto)
{
  if(proto == "UDP")
    return LO_UDP;
  if(proto == "TCP")
    return LO_TCP;
  if(proto == "UNIX")
    return LO_UNIX;
  throw TASCAR::ErrMsg("Invalid OSC protocol name \"" + proto + "\".");
}

osc_server_t::osc_server_t(const std::string& multicast,
                           const std::string& port, const std::string& proto,
                           bool verbose_)
    : osc_srv_addr(multicast), osc_srv_port(port), initialized(false),
      isactive(false), verbose(verbose_)
{
  lost = NULL;
  liblo_errflag = false;
  if(port.size() && (port != "none")) {
    if(multicast.size()) {
      lost = lo_server_thread_new_multicast(multicast.c_str(), port.c_str(),
                                            err_handler);
      // if( verbose )
      // std::cerr << "listening on multicast address \"osc.udp://" << multicast
      // << ":"<<port << "/\"" << std::endl;
      initialized = true;
    } else {
      lost = lo_server_thread_new_with_proto(port.c_str(), string2proto(proto),
                                             err_handler);
      // if( verbose )
      // std::cerr << "listening on \"osc.udp://localhost:"<<port << "/\"" <<
      // std::endl;
      initialized = true;
    }
    if((!lost) || liblo_errflag)
      throw ErrMsg("liblo error (srv_addr: \"" + multicast + "\" srv_port: \"" +
                   port + "\" " + proto + ").");
  }
  if(lost) {
    char* ctmp(lo_server_thread_get_url(lost));
    if(ctmp) {
      osc_srv_url = ctmp;
      free(ctmp);
    }
    if(verbose)
      std::cerr << "listening on \"" << osc_srv_url << "\"" << std::endl;
  }
  add_method("/sendvarsto", "ss", osc_send_variables, this);
  add_method("/sendvarsto", "sss", osc_send_variables, this);
}

int osc_server_t::dispatch_data(void* data, size_t size)
{
  lo_server srv(lo_server_thread_get_server(lost));
  return lo_server_dispatch_data(srv, data, size);
}

int osc_server_t::dispatch_data_message(const char* path, lo_message m)
{
  if(!isactive)
    return 0;
  size_t len(lo_message_length(m, path));
  char data[len + 1];
  return dispatch_data(lo_message_serialise(m, path, data, NULL), len);
}

osc_server_t::~osc_server_t()
{
  if(isactive)
    deactivate();
  if(initialized)
    lo_server_thread_free(lost);
}

void osc_server_t::set_prefix(const std::string& prefix_)
{
  prefix = prefix_;
}

void osc_server_t::add_method(const std::string& path, const char* typespec,
                              lo_method_handler h, void* user_data,
                              bool visible, bool readable,
                              const std::string& rangehint,
                              const std::string& comment)
{
  if(initialized) {
    std::string sPath(prefix + path);
    if(verbose && visible) {
      std::cerr << "added handler " << sPath;
      if(typespec)
        std::cerr << " with typespec \"" << typespec << "\"";
      std::cerr << std::endl;
    }
    if(sPath.empty())
      lo_server_thread_add_method(lost, NULL, typespec, h, user_data);
    else
      lo_server_thread_add_method(lost, sPath.c_str(), typespec, h, user_data);
    if(visible) {
      descriptor_t d;
      d.path = sPath;
      if(typespec)
        d.typespec = typespec;
      else
        d.typespec = "(any)";
      d.readable = readable;
      d.rangehint = rangehint;
      d.comment = comment;
      variables.push_back(d);
    }
  }
}

void osc_server_t::add_float(const std::string& path, float* data,
                             const std::string& range,
                             const std::string& comment)
{
  add_method(path, "f", osc_set_float, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_float, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_float, "float");
}

void osc_server_t::add_double(const std::string& path, double* data,
                              const std::string& range,
                              const std::string& comment)
{
  add_method(path, "f", osc_set_double, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_double, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_double, "double");
}

void osc_server_t::add_float_db(const std::string& path, float* data,
                                const std::string& range,
                                const std::string& comment)
{
  add_method(path, "f", osc_set_float_db, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_float_db, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_float_db, "float");
}

void osc_server_t::add_float_dbspl(const std::string& path, float* data,
                                   const std::string& range,
                                   const std::string& comment)
{
  add_method(path, "f", osc_set_float_dbspl, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_float_dbspl, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_float_dbspl, "float");
}

void osc_server_t::add_vector_float_dbspl(const std::string& path,
                                          std::vector<float>* data,
                                          const std::string& range,
                                          const std::string& comment)
{
  add_method(path, std::string(data->size(), 'f').c_str(),
             osc_set_vector_float_dbspl, data, true, false, range, comment);
}

void osc_server_t::add_vector_float_db(const std::string& path,
                                       std::vector<float>* data,
                                       const std::string& range,
                                       const std::string& comment)
{
  add_method(path, std::string(data->size(), 'f').c_str(),
             osc_set_vector_float_db, data, true, false, range, comment);
}

void osc_server_t::add_vector_float(const std::string& path,
                                    std::vector<float>* data,
                                    const std::string& range,
                                    const std::string& comment)
{
  add_method(path, std::string(data->size(), 'f').c_str(), osc_set_vector_float,
             data, true, false, range, comment);
}

void osc_server_t::add_vector_double(const std::string& path,
                                     std::vector<double>* data,
                                     const std::string& range,
                                     const std::string& comment)
{
  add_method(path, std::string(data->size(), 'f').c_str(),
             osc_set_vector_double, data, true, false, range, comment);
}

void osc_server_t::add_double_db(const std::string& path, double* data,
                                 const std::string& range,
                                 const std::string& comment)
{
  add_method(path, "f", osc_set_double_db, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_double_db, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_double_db, "double");
}

void osc_server_t::add_double_dbspl(const std::string& path, double* data,
                                    const std::string& range,
                                    const std::string& comment)
{
  add_method(path, "f", osc_set_double_dbspl, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_double_dbspl, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_double_dbspl, "double");
}

void osc_server_t::add_float_degree(const std::string& path, float* data,
                                    const std::string& range,
                                    const std::string& comment)
{
  add_method(path, "f", osc_set_float_degree, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_float_degree, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_float_degree, "float");
}

void osc_server_t::add_double_degree(const std::string& path, double* data,
                                     const std::string& range,
                                     const std::string& comment)
{
  add_method(path, "f", osc_set_double_degree, data, true, true, range,
             comment);
  add_method(path + "/get", "ss", osc_get_double_degree, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_double_degree, "double");
}

void osc_server_t::add_bool_true(const std::string& path, bool* data,
                                 const std::string& comment)
{
  add_method(path, "", osc_set_bool_true, data, true, false, "", comment);
}

void osc_server_t::add_bool_false(const std::string& path, bool* data,
                                  const std::string& comment)
{
  add_method(path, "", osc_set_bool_false, data, true, false, "", comment);
}

void osc_server_t::add_bool(const std::string& path, bool* data,
                            const std::string& comment)
{
  add_method(path, "i", osc_set_bool, data, true, true, "bool", comment);
  add_method(path + "/get", "ss", osc_get_bool, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_bool, "bool");
}

void osc_server_t::add_int(const std::string& path, int32_t* data,
                           const std::string& range, const std::string& comment)
{
  add_method(path, "i", osc_set_int32, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_int32, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_int, "int");
}

void osc_server_t::add_uint(const std::string& path, uint32_t* data,
                            const std::string& range,
                            const std::string& comment)
{
  add_method(path, "i", osc_set_uint32, data, true, true, range, comment);
  add_method(path + "/get", "ss", osc_get_uint32, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_uint, "uint");
}

void osc_server_t::add_string(const std::string& path, std::string* data,
                              const std::string& comment)
{
  add_method(path, "s", osc_set_string, data, true, true, "string", comment);
  add_method(path + "/get", "ss", osc_get_string, data, false);
  datamap[prefix+path] = data_element_t(prefix+path, data, str_get_string, "string");
}

void osc_server_t::activate()
{
  if(initialized) {
    lo_server_thread_start(lost);
    isactive = true;
    if(verbose)
      std::cerr << "server active\n";
  }
}

void osc_server_t::deactivate()
{
  if(initialized) {
    lo_server_thread_stop(lost);
    isactive = false;
    if(verbose)
      std::cerr << "server inactive\n";
  }
}

std::map<std::string, osc_server_t::descriptor_t>
osc_server_t::get_variable_map() const
{
  std::map<std::string, descriptor_t> vars;
  for(const auto& var : variables)
    vars[var.path + var.typespec] = var;
  return vars;
}

std::string osc_server_t::list_variables() const
{
  auto vars = get_variable_map();
  std::string rv;
  for(const auto& v : vars)
    rv += v.second.path + "  (" + v.second.typespec + ")" +
          (v.second.readable ? " r " : " ") + v.second.rangehint + " " +
          v.second.comment + "\n";
  return rv;
}

const std::string& osc_server_t::get_prefix() const
{
  return prefix;
}

void osc_server_t::send_variable_list(const std::string& url,
                                      const std::string& path,
                                      const std::string& prefix) const
{
  lo_address target = lo_address_new_from_url(url.c_str());
  if(!target)
    return;
  lo_send(target, (path + "/begin").c_str(), "");
  for(const auto& var : variables)
    if(prefix.empty() || (var.path.find(prefix) == 0))
      lo_send(target, path.c_str(), "ssiss", var.path.c_str(),
              var.typespec.c_str(), var.readable, var.rangehint.c_str(),
              var.comment.c_str());
  lo_send(target, (path + "/end").c_str(), "");
  lo_address_free(target);
}

TASCAR::msg_t::msg_t(tsccfg::node_t e)
    : TASCAR::xml_element_t(e), msg(lo_message_new())
{
  GET_ATTRIBUTE(path, "", "OSC path name");
  for(auto& sne : tsccfg::node_get_children(e, "f")) {
    TASCAR::xml_element_t tsne(sne);
    double v(0);
    tsne.GET_ATTRIBUTE(v, "", "float value");
    lo_message_add_float(msg, v);
  }
  for(auto& sne : tsccfg::node_get_children(e, "i")) {
    TASCAR::xml_element_t tsne(sne);
    int32_t v(0);
    tsne.GET_ATTRIBUTE(v, "", "int value");
    lo_message_add_int32(msg, v);
  }
  for(auto& sne : tsccfg::node_get_children(e, "s")) {
    TASCAR::xml_element_t tsne(sne);
    std::string v("");
    tsne.GET_ATTRIBUTE(v, "", "string value");
    lo_message_add_string(msg, v.c_str());
  }
}

TASCAR::msg_t::~msg_t()
{
  lo_message_free(msg);
}

std::string osc_server_t::get_vars_as_json_rg(
    std::string prefix,
    std::map<std::string, data_element_t>::const_iterator& ibegin,
    std::map<std::string, data_element_t>::const_iterator iend,
    bool asstring) const
{
  std::string r = "{";
  std::string lastprefix;
  if((prefix.size() > 0) && (prefix[prefix.size() - 1] == '/'))
    prefix.erase(prefix.size() - 1);
  for(auto it = ibegin; it != iend; ++it) {
    if(prefix.empty() || (it->second.path.find(prefix) == 0)) {
      std::string vpf = it->second.prefix;
      size_t pos = vpf.find(prefix);
      if(pos == 0)
        vpf.erase(0, prefix.size());
      if(vpf[0] == '/')
        vpf.erase(0, 1);
      if(!vpf.empty()) {
        r += "\"" + vpf + "\":" +
             get_vars_as_json_rg(it->second.prefix, it, iend, asstring) + ",";
      } else {
        if(asstring || (it->second.type == "string"))
          r += "\"" + it->second.name + "\":\"" + it->second.getstr() + "\",";
        else
          r += "\"" + it->second.name + "\":" + it->second.getstr() + ",";
      }
      ibegin = it;
      lastprefix = vpf;
    }
  }
  if(r[r.size() - 1] == ',')
    r.erase(r.size() - 1);
  r += "}";
  return r;
}

std::string osc_server_t::get_vars_as_json(std::string prefix,
                                           bool asstring) const
{
  auto it = datamap.begin();
  return get_vars_as_json_rg(prefix, it, datamap.end(), asstring);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
