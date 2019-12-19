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
#include "osc_helper.h"
#include "audiostates.h"

namespace TASCAR {

  namespace levelmeter {
    enum weight_t {
      Z,
      bandpass,
      C,
      A
    };
  };

  class globalconfig_t {
  public:
    globalconfig_t();
    double operator()(const std::string&,double) const;
    std::string operator()(const std::string&,const std::string&) const;
    void forceoverwrite(const std::string&,const std::string&);
  private:
    void readconfig(const std::string& fname);
    void readconfig(const std::string& prefix, xmlpp::Element* e);
    std::map<std::string,std::string> cfg;
  };

  std::string to_string( double x );
  std::string to_string( const TASCAR::levelmeter::weight_t& value );

  std::string days_to_string( double x );

  void add_warning( std::string msg, xmlpp::Element* e = NULL );
  
  std::string tscbasename( const std::string& s );

  std::string default_string(const std::string& src,const std::string& def);

  extern std::map<xmlpp::Element*,std::map<std::string,std::string> > attribute_list;
  extern std::vector<std::string> warnings;

  extern globalconfig_t config;

  class xml_element_t {
  public:
    xml_element_t(xmlpp::Element*);
    virtual ~xml_element_t();
    bool has_attribute( const std::string& name) const;
    std::string get_element_name() const { return e->get_name(); };
    // get attributes:
    void get_attribute(const std::string& name,std::string& value);
    void get_attribute(const std::string& name,double& value);
    void get_attribute(const std::string& name,float& value);
    void get_attribute(const std::string& name,uint32_t& value);
    void get_attribute(const std::string& name,int32_t& value);
    void get_attribute(const std::string& name,uint64_t& value);
    void get_attribute(const std::string& name,int64_t& value);
    void get_attribute_bool(const std::string& name,bool& value);
    void get_attribute_db(const std::string& name,double& value);
    void get_attribute_dbspl(const std::string& name,double& value);
    void get_attribute_db_float(const std::string& name,float& value);
    void get_attribute_deg(const std::string& name,double& value);
    void get_attribute(const std::string& name,TASCAR::pos_t& value);
    void get_attribute(const std::string& name,TASCAR::levelmeter::weight_t& value);
    void get_attribute(const std::string& name,std::vector<TASCAR::pos_t>& value);
    void get_attribute(const std::string& name,std::vector<std::string>& value);
    void get_attribute(const std::string& name,std::vector<double>& value);
    void get_attribute(const std::string& name,std::vector<float>& value);
    void get_attribute(const std::string& name,std::vector<int32_t>& value);
    void get_attribute_bits(const std::string& name,uint32_t& value);
    // set attributes:
    void set_attribute_bool(const std::string& name,bool value);
    void set_attribute_db(const std::string& name,double value);
    void set_attribute_dbspl(const std::string& name,double value);
    void set_attribute(const std::string& name,const std::string& value);
    void set_attribute(const std::string& name,double value);
    void set_attribute_deg(const std::string& name,double value);
    void set_attribute(const std::string& name,int32_t value);
    void set_attribute(const std::string& name,uint32_t value);
    void set_attribute_bits(const std::string& name,uint32_t value);
    void set_attribute(const std::string& name,int64_t value);
    void set_attribute(const std::string& name,uint64_t value);
    void set_attribute(const std::string& name,const TASCAR::pos_t& value);
    void set_attribute(const std::string& name,const TASCAR::levelmeter::weight_t& value);
    void set_attribute(const std::string& name,const std::vector<TASCAR::pos_t>& value);
    void set_attribute(const std::string& name,const std::vector<std::string>& value);
    void set_attribute(const std::string& name,const std::vector<double>& value);
    void set_attribute(const std::string& name,const std::vector<float>& value);
    void set_attribute(const std::string& name,const std::vector<int32_t>& value);
    xmlpp::Element* find_or_add_child(const std::string& name);
    xmlpp::Element* e;
    std::vector<std::string> get_unused_attributes() const;
    virtual void validate_attributes(std::string&) const;
    size_t hash(const std::vector<std::string>& attributes, bool test_children = false) const;
  protected:
    // book keeping
    //std::map<std::string,std::string> attributelist;
  };

  class scene_node_base_t : public xml_element_t, public audiostates_t {
  public:
    scene_node_base_t(xmlpp::Element*);
    virtual ~scene_node_base_t();
  };

  std::string env_expand( std::string s );

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
    xml_doc_t(const std::string& filename,load_type_t t);
    virtual void save(const std::string& filename);
    //protected:
    xmlpp::DomParser domp;
    xmlpp::Document* doc;
  };

  class msg_t : public TASCAR::xml_element_t {
  public:
    msg_t( xmlpp::Element* );
    ~msg_t();
    std::string path;
    lo_message msg;
  private:
    msg_t( const msg_t& );
  };
  
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,float& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,uint32_t& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,int32_t& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,uint64_t& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,int64_t& value);
void get_attribute_value_bool(xmlpp::Element* elem,const std::string& name,bool& value);
void get_attribute_value_db(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value_dbspl(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value_db_float(xmlpp::Element* elem,const std::string& name,float& value);
void get_attribute_value_deg(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,TASCAR::pos_t& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<TASCAR::pos_t>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<std::string>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<double>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<float>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<int32_t>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,TASCAR::levelmeter::weight_t& value);

void set_attribute_bool(xmlpp::Element* elem,const std::string& name,bool value);
void set_attribute_db(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_dbspl(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_double(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_uint32(xmlpp::Element* elem,const std::string& name,uint32_t value);
void set_attribute_int32(xmlpp::Element* elem,const std::string& name,int32_t value);
void set_attribute_uint64(xmlpp::Element* elem,const std::string& name,uint64_t value);
void set_attribute_int64(xmlpp::Element* elem,const std::string& name,int64_t value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::pos_t& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<TASCAR::pos_t>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<std::string>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<double>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<float>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<int32_t>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::levelmeter::weight_t& value);

#define GET_ATTRIBUTE(x) get_attribute(#x,x)
#define SET_ATTRIBUTE(x) set_attribute(#x,x)
#define GET_ATTRIBUTE_DB(x) get_attribute_db(#x,x)
#define SET_ATTRIBUTE_DB(x) set_attribute_db(#x,x)
#define GET_ATTRIBUTE_DBSPL(x) get_attribute_dbspl(#x,x)
#define SET_ATTRIBUTE_DBSPL(x) set_attribute_dbspl(#x,x)
#define GET_ATTRIBUTE_DEG(x) get_attribute_deg(#x,x)
#define SET_ATTRIBUTE_DEG(x) set_attribute_deg(#x,x)
#define GET_ATTRIBUTE_BOOL(x) get_attribute_bool(#x,x)
#define SET_ATTRIBUTE_BOOL(x) set_attribute_bool(#x,x)
#define GET_ATTRIBUTE_BITS(x) get_attribute_bits(#x,x)

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
