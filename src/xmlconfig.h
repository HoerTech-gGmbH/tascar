#ifndef XMLCONFIG_H
#define XMLCONFIG_H

#include <libxml++/libxml++.h>
#include <stdint.h>
#include "coordinates.h"

namespace TASCAR {

  class xml_element_t {
  public:
    xml_element_t(xmlpp::Element*);
    virtual ~xml_element_t();
    std::string get_element_name() const { return e->get_name(); };
    // get attributes:
    void get_attribute(const std::string& name,std::string& value);
    void get_attribute(const std::string& name,double& value);
    void get_attribute(const std::string& name,unsigned int& value);
    void get_attribute_bool(const std::string& name,bool& value);
    void get_attribute_db(const std::string& name,double& value);
    void get_attribute_db_float(const std::string& name,float& value);
    void get_attribute_deg(const std::string& name,double& value);
    void get_attribute(const std::string& name,TASCAR::pos_t& value);
    // set attributes:
    void set_attribute_bool(const std::string& name,bool value);
    void set_attribute_db(const std::string& name,double value);
    void set_attribute(const std::string& name,const std::string& value);
    void set_attribute(const std::string& name,double value);
    void set_attribute(const std::string& name,unsigned int value);
    void set_attribute(const std::string& name,const TASCAR::pos_t& value);
    virtual void write_xml() = 0;
    xmlpp::Element* find_or_add_child(const std::string& name);
    xmlpp::Element* e;
  };

  class scene_node_base_t : public xml_element_t {
  public:
    scene_node_base_t(xmlpp::Element*);
    virtual ~scene_node_base_t();
    virtual void prepare(double fs, uint32_t fragsize) = 0;
  };

  std::string env_expand( std::string s );

}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,unsigned int& value);
void get_attribute_value_bool(xmlpp::Element* elem,const std::string& name,bool& value);
void get_attribute_value_db(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value_db_float(xmlpp::Element* elem,const std::string& name,float& value);
void get_attribute_value_deg(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,TASCAR::pos_t& value);
void set_attribute_bool(xmlpp::Element* elem,const std::string& name,bool value);
void set_attribute_db(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_double(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_uint(xmlpp::Element* elem,const std::string& name,unsigned int value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::pos_t& value);

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
