#include "defs.h"
#include "xmlconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

TASCAR::scene_node_base_t::scene_node_base_t()
  : p_xml_element(NULL)
{
}

TASCAR::scene_node_base_t::~scene_node_base_t()
{
}

void TASCAR::scene_node_base_t::read_xml(xmlpp::Element* e)
{
  p_xml_element = e;
}

void TASCAR::scene_node_base_t::get_value(const std::string& name,double& value)
{
  if( p_xml_element )
    get_attribute_value(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::get_value(const std::string& name,unsigned int& value)
{
  if( p_xml_element )
    get_attribute_value(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::get_value_bool(const std::string& name,bool& value)
{
  if( p_xml_element )
    get_attribute_value_bool(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::get_value_db(const std::string& name,double& value)
{
  if( p_xml_element )
    get_attribute_value_db(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::get_value_db_float(const std::string& name,float& value)
{
  if( p_xml_element )
    get_attribute_value_db_float(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::get_value_deg(const std::string& name,double& value)
{
  if( p_xml_element )
    get_attribute_value_deg(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::get_value(const std::string& name,TASCAR::pos_t& value)
{
  if( p_xml_element )
    get_attribute_value(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::set_bool(const std::string& name,bool value)
{
  if( p_xml_element )
    set_attribute_bool(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::set_db(const std::string& name,double value)
{
  if( p_xml_element )
    set_attribute_db(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::set_double(const std::string& name,double value)
{
  if( p_xml_element )
    set_attribute_double(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::set_uint(const std::string& name,unsigned int value)
{
  if( p_xml_element )
    set_attribute_uint(p_xml_element,name,value);
}

void TASCAR::scene_node_base_t::set_value(const std::string& name,const TASCAR::pos_t& value)
{
  if( p_xml_element )
    set_attribute_value(p_xml_element,name,value);
}

std::string localgetenv(const std::string& env)
{
  if( char* s = getenv(env.c_str()) )
    return s;
  return "";
}

std::string TASCAR::env_expand( std::string s )
{
  size_t spos;
  while( (spos = s.find("${")) != std::string::npos ){
    size_t epos(s.find("}",spos));
    if( epos == std::string::npos )
      epos = s.size();
    std::string env(s.substr(spos+2,epos-spos-2));
    s.replace(spos,epos-spos+1,localgetenv(env));
  }
  return s;
}


void set_attribute_uint(xmlpp::Element* elem,const std::string& name,unsigned int value)
{
  char ctmp[1024];
  sprintf(ctmp,"%d",value);
  elem->set_attribute(name,ctmp);
}

void set_attribute_bool(xmlpp::Element* elem,const std::string& name,bool value)
{
  if( value )
    elem->set_attribute(name,"true");
  else
    elem->set_attribute(name,"false");
}

void set_attribute_double(xmlpp::Element* elem,const std::string& name,double value)
{
  char ctmp[1024];
  sprintf(ctmp,"%1.12g",value);
  elem->set_attribute(name,ctmp);
}

void set_attribute_db(xmlpp::Element* elem,const std::string& name,double value)
{
  char ctmp[1024];
  sprintf(ctmp,"%1.12g",20.0*log10(value));
  elem->set_attribute(name,ctmp);
}


void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::pos_t& value)
{
  char ctmp[1024];
  sprintf(ctmp,"%g %g %g",value.x, value.y, value.z);
  elem->set_attribute(name,ctmp);
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,TASCAR::pos_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  TASCAR::pos_t tmpv;
  if( sscanf(attv.c_str(),"%lf%lf%lf",&(tmpv.x),&(tmpv.y),&(tmpv.z))==3 ){
    value = tmpv;
  }
}

void get_attribute_value_deg(xmlpp::Element* elem,const std::string& name,double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = DEG2RAD*tmpv;
}

void get_attribute_value_db(xmlpp::Element* elem,const std::string& name,double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = pow(10.0,0.05*tmpv);
}

void get_attribute_value_db_float(xmlpp::Element* elem,const std::string& name,float& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = pow(10.0,0.05*tmpv);
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,unsigned int& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = tmpv;
}


void get_attribute_value_bool(xmlpp::Element* elem,const std::string& name,bool& value)
{
  std::string attv(elem->get_attribute_value(name));
  if( attv.size() )
    value = (attv == "true");
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
