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
#ifndef XMLCONFIG_H
#define XMLCONFIG_H

#include "coordinates.h"

//#define USEPUGIXML

#ifdef USEPUGIXML

#include <pugixml.hpp>

namespace tsccfg {
  typedef pugi::xml_node node_t;
}
#else // not using pigixml

#include <libxml++/libxml++.h>

namespace tsccfg {
  typedef xmlpp::Element* node_t;
}

#endif // xml library

namespace tsccfg {

  std::vector<tsccfg::node_t> node_get_children(tsccfg::node_t&,
                                                const std::string& name = "");
  const std::vector<tsccfg::node_t> node_get_children(const tsccfg::node_t&);
  tsccfg::node_t node_add_child(tsccfg::node_t&, const std::string&);
  std::string node_get_attribute_value(const tsccfg::node_t&,
                                       const std::string& n);
  std::string node_get_name(const tsccfg::node_t&);
  std::string node_get_path(const tsccfg::node_t&);
  void node_set_attribute(tsccfg::node_t&, const std::string& n,
                          const std::string& v);
  bool node_has_attribute(const tsccfg::node_t& e, const std::string& name);
  void node_get_and_register_attribute(tsccfg::node_t& e,
                                       const std::string& name,
                                       std::string& value,
                                       const std::string& info);
  std::string node_get_text(tsccfg::node_t n, const std::string& child = "");

  double node_xpath_to_number(tsccfg::node_t,const std::string& path);

} // namespace tsccfg

namespace TASCAR {

  namespace levelmeter {
    enum weight_t { Z, bandpass, C, A };
  };

  class globalconfig_t {
  public:
    globalconfig_t();
    double operator()(const std::string&, double) const;
    std::string operator()(const std::string&, const std::string&) const;
    void forceoverwrite(const std::string&, const std::string&);

  private:
    void readconfig(const std::string& fname);
    void readconfig(const std::string& prefix, tsccfg::node_t& e);
    std::map<std::string, std::string> cfg;
  };

  // get a TASCAR unique identifier, valid within one program start:
  std::string get_tuid();

  std::string strrep(std::string s, const std::string& pat,
                     const std::string& rep);

  std::string to_string(double x);
  std::string to_string(float x);
  std::string to_string(uint32_t x);
  std::string to_string(int32_t x);
  std::string to_string(const TASCAR::pos_t& x);
  std::string to_string(const TASCAR::zyx_euler_t& x);
  std::string to_string_deg(const TASCAR::zyx_euler_t& x);
  std::string to_string(const TASCAR::levelmeter::weight_t& value);
  std::string to_string(const std::vector<int>& value);
  std::string to_string(const std::vector<double>& value);
  std::string to_string(const std::vector<float>& value);
  std::string to_string(const std::vector<TASCAR::pos_t>& value);
  std::string to_string_bits(uint32_t value);
  std::string to_string_db(double value);
  std::string to_string_dbspl(double value);
  std::string to_string_db(float value);
  std::string to_string_dbspl(float value);
  std::string to_string_bool(bool value);

  std::string days_to_string(double x);

  void add_warning(std::string msg);
  void add_warning(std::string msg, const tsccfg::node_t& e);

  std::string tscbasename(const std::string& s);

  std::string default_string(const std::string& src, const std::string& def);

  struct cfg_var_desc_t {
    std::string name;
    std::string type;
    std::string defaultval;
    std::string unit;
    std::string info;
  };

  struct cfg_node_desc_t {
    std::string category;
    std::string type;
    std::map<std::string, cfg_var_desc_t> vars;
  };

  extern std::map<std::string, cfg_node_desc_t> attribute_list;
  extern std::vector<std::string> warnings;

  extern globalconfig_t config;

  class xml_element_t {
  public:
    xml_element_t(tsccfg::node_t);
    virtual ~xml_element_t();
    bool has_attribute(const std::string& name) const;
    std::string get_element_name() const;
    // get attributes:
    void get_attribute(const std::string& name, std::string& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, double& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, float& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, uint32_t& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, int32_t& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, uint64_t& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, int64_t& value,
                       const std::string& unit, const std::string& info);
    void get_attribute_bool(const std::string& name, bool& value,
                            const std::string& unit, const std::string& info);
    void get_attribute_db(const std::string& name, double& value,
                          const std::string& info);
    void get_attribute_dbspl(const std::string& name, double& value,
                             const std::string& info);
    void get_attribute_dbspl(const std::string& name, float& value,
                             const std::string& info);
    void get_attribute_db(const std::string& name, float& value,
                          const std::string& info);
    void get_attribute_deg(const std::string& name, double& value,
                           const std::string& info);
    void get_attribute(const std::string& name, TASCAR::pos_t& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, TASCAR::zyx_euler_t& value,
                       const std::string& info);
    void get_attribute(const std::string& name,
                       TASCAR::levelmeter::weight_t& value,
                       const std::string& info);
    void get_attribute(const std::string& name,
                       std::vector<TASCAR::pos_t>& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, std::vector<std::string>& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, std::vector<double>& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, std::vector<float>& value,
                       const std::string& unit, const std::string& info);
    void get_attribute(const std::string& name, std::vector<int32_t>& value,
                       const std::string& unit, const std::string& info);
    void get_attribute_bits(const std::string& name, uint32_t& value,
                            const std::string& info);
    // set attributes:
    void set_attribute_bool(const std::string& name, bool value);
    void set_attribute_db(const std::string& name, double value);
    void set_attribute_dbspl(const std::string& name, double value);
    void set_attribute(const std::string& name, const std::string& value);
    void set_attribute(const std::string& name, double value);
    void set_attribute_deg(const std::string& name, double value);
    void set_attribute(const std::string& name, int32_t value);
    void set_attribute(const std::string& name, uint32_t value);
    void set_attribute_bits(const std::string& name, uint32_t value);
    void set_attribute(const std::string& name, int64_t value);
    void set_attribute(const std::string& name, uint64_t value);
    void set_attribute(const std::string& name, const TASCAR::pos_t& value);
    void set_attribute(const std::string& name,
                       const TASCAR::zyx_euler_t& value);
    void set_attribute(const std::string& name,
                       const TASCAR::levelmeter::weight_t& value);
    void set_attribute(const std::string& name,
                       const std::vector<TASCAR::pos_t>& value);
    void set_attribute(const std::string& name,
                       const std::vector<std::string>& value);
    void set_attribute(const std::string& name,
                       const std::vector<double>& value);
    void set_attribute(const std::string& name,
                       const std::vector<float>& value);
    void set_attribute(const std::string& name,
                       const std::vector<int32_t>& value);
    tsccfg::node_t find_or_add_child(const std::string& name);
    tsccfg::node_t e;
    std::vector<std::string> get_unused_attributes() const;
    virtual void validate_attributes(std::string&) const;
    size_t hash(const std::vector<std::string>& attributes,
                bool test_children = false) const;
    std::vector<std::string> get_attributes() const;

  protected:
  };

  std::string env_expand(std::string s);

  std::vector<TASCAR::pos_t> str2vecpos(const std::string& s);
  std::vector<std::string> str2vecstr(const std::string& s);
  std::string vecstr2str(const std::vector<std::string>& s);
  std::vector<double> str2vecdouble(const std::string& s);
  std::vector<float> str2vecfloat(const std::string& s);
  std::vector<int32_t> str2vecint(const std::string& s);

  class xml_doc_t {
  public:
    enum load_type_t { LOAD_FILE, LOAD_STRING };
    xml_doc_t();
    xml_doc_t(const std::string& filename, load_type_t t);
    xml_doc_t(tsccfg::node_t);
    virtual ~xml_doc_t();
    virtual void save(const std::string& filename);
    std::string save_to_string();
    tsccfg::node_t root;
#ifdef USEPUGIXML
    pugi::xml_document doc;
#else
    xmlpp::DomParser domp;
    xmlpp::Document* doc;

    bool freedoc;
#endif
  private:
    tsccfg::node_t get_root_node();
  };

} // namespace TASCAR

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::string& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         double& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         float& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         uint32_t& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         int32_t& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         uint64_t& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         int64_t& value);
void get_attribute_value_bool(const tsccfg::node_t& elem,
                              const std::string& name, bool& value);
void get_attribute_value_db(const tsccfg::node_t& elem, const std::string& name,
                            double& value);
void get_attribute_value_dbspl(const tsccfg::node_t& elem,
                               const std::string& name, double& value);
void get_attribute_value_dbspl_float(const tsccfg::node_t& elem,
                                     const std::string& name, float& value);
void get_attribute_value_db_float(const tsccfg::node_t& elem,
                                  const std::string& name, float& value);
void get_attribute_value_deg(const tsccfg::node_t& elem,
                             const std::string& name, double& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         TASCAR::pos_t& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         TASCAR::zyx_euler_t& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<TASCAR::pos_t>& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<std::string>& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<double>& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<float>& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<int32_t>& value);
void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         TASCAR::levelmeter::weight_t& value);

void set_attribute_bool(tsccfg::node_t& elem, const std::string& name,
                        bool value);
void set_attribute_db(tsccfg::node_t& elem, const std::string& name,
                      double value);
void set_attribute_dbspl(tsccfg::node_t& elem, const std::string& name,
                         double value);
void set_attribute_double(tsccfg::node_t& elem, const std::string& name,
                          double value);
void set_attribute_uint32(tsccfg::node_t& elem, const std::string& name,
                          uint32_t value);
void set_attribute_int32(tsccfg::node_t& elem, const std::string& name,
                         int32_t value);
void set_attribute_uint64(tsccfg::node_t& elem, const std::string& name,
                          uint64_t value);
void set_attribute_int64(tsccfg::node_t& elem, const std::string& name,
                         int64_t value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const TASCAR::pos_t& value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const TASCAR::zyx_euler_t& value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<TASCAR::pos_t>& value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<std::string>& value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<double>& value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<float>& value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<int32_t>& value);
void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const TASCAR::levelmeter::weight_t& value);

#define GET_ATTRIBUTE(x, u, i) get_attribute(#x, x, u, i)
#define GET_ATTRIBUTE_(x) get_attribute(#x, x, "", "undocumented")
#define GET_ATTRIBUTE_NOUNIT(x, i) get_attribute(#x, x, i)
#define GET_ATTRIBUTE_NOUNIT_(x) get_attribute(#x, x, "undocumented")
#define SET_ATTRIBUTE(x) set_attribute(#x, x)
#define GET_ATTRIBUTE_DB(x, i) get_attribute_db(#x, x, i)
#define GET_ATTRIBUTE_DB_(x) get_attribute_db(#x, x, "undocumented")
#define SET_ATTRIBUTE_DB(x) set_attribute_db(#x, x)
#define GET_ATTRIBUTE_DBSPL(x, i) get_attribute_dbspl(#x, x, i)
#define GET_ATTRIBUTE_DBSPL_(x) get_attribute_dbspl(#x, x, "undocumented")
#define SET_ATTRIBUTE_DBSPL(x) set_attribute_dbspl(#x, x)
#define GET_ATTRIBUTE_DEG(x, i) get_attribute_deg(#x, x, i)
#define GET_ATTRIBUTE_DEG_(x) get_attribute_deg(#x, x, "undocumented")
#define SET_ATTRIBUTE_DEG(x) set_attribute_deg(#x, x)
#define GET_ATTRIBUTE_BOOL(x, i) get_attribute_bool(#x, x, "", i)
#define GET_ATTRIBUTE_BOOL_(x) get_attribute_bool(#x, x, "", "undocumented")
#define SET_ATTRIBUTE_BOOL(x) set_attribute_bool(#x, x)
#define GET_ATTRIBUTE_BITS(x, i) get_attribute_bits(#x, x, i)
#define GET_ATTRIBUTE_BITS_(x) get_attribute_bits(#x, x, "undocumented")

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
