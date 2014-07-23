#ifndef XMLCONFIG_H
#define XMLCONFIG_H

#include <libxml++/libxml++.h>
#include <stdint.h>
#include "coordinates.h"

namespace TASCAR {

  class scene_node_base_t {
  public:
    scene_node_base_t(){};
    virtual ~scene_node_base_t(){};
    virtual void read_xml(xmlpp::Element* e) = 0;
    virtual void write_xml(xmlpp::Element* e,bool help_comments=false) = 0;
    //virtual std::string print(const std::string& prefix="") = 0;
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
