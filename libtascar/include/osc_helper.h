/**
 * @file osc_helper.h
 * @ingroup libtascar
 * @brief helper classes for OSC
 * @author Giso Grimm
 * @date 2012
 */
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
 */

#ifndef OSC_HELPER_H
#define OSC_HELPER_H

#include "tscconfig.h"
#if defined(_WIN32) && !defined(WIN32)
#define WIN32 _WIN32 // liblo needs WIN32 defined in order to detect Windows
#endif
#include <lo/lo.h>
#include <string>
#include <vector>

namespace TASCAR {

  /// OSC server
  class osc_server_t {
  public:
    class descriptor_t {
    public:
      std::string path;
      std::string typespec;
    };
    osc_server_t(const std::string& multicast, const std::string& port,
                 const std::string& proto, bool verbose = true);
    ~osc_server_t();
    void set_prefix(const std::string& prefix);
    const std::string& get_prefix() const;
    /**
       \brief Register a method in the OSC server
       \param path OSC path (a prefix may be added internally, see \ref
       set_prefix() ) \param typespec OSC types, a string consisting of the
       characters f(loat) d(ouble) i(nteger) s(tring) \param h Method handler
       \param user_data Pointer to user data
     */
    void add_method(const std::string& path, const char* typespec,
                    lo_method_handler h, void* user_data);
    /** \brief Register a double variable for OSC access
        \param path OSC path
        \param data Pointer to data
     */
    void add_double(const std::string& path, double* data);
    /** \brief Register a double variable for OSC access, convert from dB values

        In coming messages will be converted from dB to linear representation.

        \param path OSC path
        \param data Pointer to data
     */
    void add_double_db(const std::string& path, double* data);
    /** \brief Register a double variable for OSC access, convert from dB SPL
       values

        In coming messages will be converted from dB SPL to Pascal
       representation.

        \param path OSC path
        \param data Pointer to data
     */
    void add_double_dbspl(const std::string& path, double* data);
    /** \brief Register a double variable for OSC access, convert from degree
       values

        In coming messages will be converted from degrees to radians.

        \param path OSC path
        \param data Pointer to data
     */
    void add_double_degree(const std::string& path, double* data);
    /** \brief Register a float variable for OSC access
        \param path OSC path
        \param data Pointer to data
     */
    void add_float(const std::string& path, float* data);
    /** \brief Register a float variable for OSC access, convert from dB values

        In coming messages will be converted from dB to linear representation.

        \param path OSC path
        \param data Pointer to data
     */
    void add_float_db(const std::string& path, float* data);
    void add_float_dbspl(const std::string& path, float* data);
    /** \brief Register a float variable for OSC access, convert from degree
       values

        In coming messages will be converted from degrees to radians.

        \param path OSC path
        \param data Pointer to data
     */
    void add_float_degree(const std::string& path, float* data);
    /** \brief Register a vector of floats variable for OSC access

        The dimension of the vector specifies the length of the
        message. Sending a message of different size will not have any
        effect, i.e., the callback handler will not change the size of
        the data.

        \param path OSC path
        \param data Pointer to data
     */
    void add_vector_float(const std::string& path, std::vector<float>* data);
    /** \brief Register a vector of doubles variable for OSC access

        The dimension of the vector specifies the length of the
        message. Sending a message of different size will not have any
        effect, i.e., the callback handler will not change the size of
        the data.

        \param path OSC path
        \param data Pointer to data
     */
    void add_vector_double(const std::string& path, std::vector<double>* data);
    /** \brief Register a vector of floats variable for OSC access as dB SPL

        The dimension of the vector specifies the length of the
        message. Sending a message of different size will not have any
        effect, i.e., the callback handler will not change the size of
        the data.

        \param path OSC path
        \param data Pointer to data
     */
    void add_vector_float_dbspl(const std::string& path,
                                std::vector<float>* data);
    /** \brief Register a vector of floats variable for OSC access as dB (e.g. for gains)

        The dimension of the vector specifies the length of the
        message. Sending a message of different size will not have any
        effect, i.e., the callback handler will not change the size of
        the data.

        \param path OSC path
        \param data Pointer to data
     */
    void add_vector_float_db(const std::string& path,
                                std::vector<float>* data);
    void add_bool_true(const std::string& path, bool* data);
    void add_bool_false(const std::string& path, bool* data);
    void add_bool(const std::string& path, bool* data);
    void add_int(const std::string& path, int32_t* data);
    void add_uint(const std::string& path, uint32_t* data);
    void add_string(const std::string& path, std::string* data);
    void activate();
    void deactivate();
    std::string list_variables() const;
    int dispatch_data(void* data, size_t size);
    int dispatch_data_message(const char* path, lo_message m);
    int get_srv_port() const { return lo_server_thread_get_port(lost); };
    std::vector<descriptor_t> variables;
    const std::string osc_srv_addr;
    const std::string osc_srv_port;
    const std::string& get_srv_url() const { return osc_srv_url; };

  private:
    std::string osc_srv_url;
    std::string prefix;
    lo_server_thread lost;
    bool initialized;
    bool isactive;
    bool verbose;
  };

  class msg_t : public TASCAR::xml_element_t {
  public:
    msg_t(tsccfg::node_t);
    ~msg_t();
    std::string path;
    lo_message msg;

  private:
    msg_t(const msg_t&);
  };

}; // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
