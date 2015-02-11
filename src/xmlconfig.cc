#include "xmlconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include "errorhandling.h"

std::string TASCAR::default_string(const std::string& src,const std::string& def)
{
  if( src.empty() )
    return def;
  return src;
}

TASCAR::scene_node_base_t::scene_node_base_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc)
{
}

TASCAR::scene_node_base_t::~scene_node_base_t()
{
}

TASCAR::xml_element_t::xml_element_t(xmlpp::Element* src)
  : e(src)
{
  if( !e )
    throw TASCAR::ErrMsg("Invalid NULL element pointer.");
}

TASCAR::xml_element_t::~xml_element_t()
{
}

xmlpp::Element* TASCAR::xml_element_t::find_or_add_child(const std::string& name)
{
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == name ))
      return sne;
  }
  return e->add_child(name);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,std::string& value)
{
  value = e->get_attribute_value(name);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,double& value)
{
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,unsigned int& value)
{
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_bool(const std::string& name,bool& value)
{
  get_attribute_value_bool(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_db(const std::string& name,double& value)
{
  get_attribute_value_db(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_db_float(const std::string& name,float& value)
{
  get_attribute_value_db_float(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_deg(const std::string& name,double& value)
{
  get_attribute_value_deg(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,TASCAR::pos_t& value)
{
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,std::vector<TASCAR::pos_t>& value)
{
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::set_attribute_bool(const std::string& name,bool value)
{
  ::set_attribute_bool(e,name,value);
}

void TASCAR::xml_element_t::set_attribute_db(const std::string& name,double value)
{
  ::set_attribute_db(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const std::string& value)
{
  e->set_attribute(name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,double value)
{
  set_attribute_double(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,unsigned int value)
{
  set_attribute_uint(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const TASCAR::pos_t& value)
{
  set_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const std::vector<TASCAR::pos_t>& value)
{
  set_attribute_value(e,name,value);
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
  elem->set_attribute(name,value.print_cart(" "));
}

void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<TASCAR::pos_t>& value)
{
  std::string s;
  for(std::vector<TASCAR::pos_t>::const_iterator i_vert=value.begin();i_vert!=value.end();++i_vert){
    if( i_vert != value.begin() )
      s += " ";
    s += i_vert->print_cart(" ");
  }
  elem->set_attribute(name,s);
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

std::vector<TASCAR::pos_t> TASCAR::str2vecpos(const std::string& s)
{
  std::vector<TASCAR::pos_t> value;
  std::stringstream ptxt(s);
  while( ptxt.good() ){
    TASCAR::pos_t p;
    ptxt >> p.x;
    if( ptxt.good() ){
      ptxt >> p.y;
      if( ptxt.good() ){
        ptxt >> p.z;
        value.push_back(p);
        //DEBUGS(ptxt.str());
        //DEBUGS(p.print_cart());
      }
    }
  }
  return value;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<TASCAR::pos_t>& value)
{
  value = TASCAR::str2vecpos(elem->get_attribute_value(name));
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
