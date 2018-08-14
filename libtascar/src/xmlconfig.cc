#include "xmlconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include "errorhandling.h"

namespace TASCAR {
  std::map<xmlpp::Element*,std::map<std::string,std::string> > attribute_list;
  std::vector<std::string> warnings;
}

void TASCAR::add_warning( std::string msg, xmlpp::Element* e )
{
  if( e ){
    char ctmp[256];
    sprintf(ctmp,"Line %d: ",e->get_line());
    msg = ctmp+msg;
  }
  warnings.push_back( msg );
  std::cerr << "Warning: " << msg << std::endl;
}

std::string TASCAR::tscbasename( const std::string& s )
{
  return s.substr(s.rfind("/")+1);
}

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
  attribute_list[e][name] = "string";
  value = e->get_attribute_value(name);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,double& value)
{
  attribute_list[e][name] = "double";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,float& value)
{
  attribute_list[e][name] = "float";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,uint32_t& value)
{
  attribute_list[e][name] = "uint32";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,int32_t& value)
{
  attribute_list[e][name] = "int32";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,uint64_t& value)
{
  attribute_list[e][name] = "uint64";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,int64_t& value)
{
  attribute_list[e][name] = "int64";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_bits(const std::string& name,uint32_t& value)
{
  attribute_list[e][name] = "bits32";
  std::vector<int32_t> bits;
  get_attribute(name,bits);
  if( bits.size() ){
    value = 0;
    for(uint32_t k=0;k<bits.size();++k){
      if( bits[k] < 32 )
        value |= (1<<bits[k]);
    }
  }
}

void TASCAR::xml_element_t::get_attribute_bool(const std::string& name,bool& value)
{
  attribute_list[e][name] = "bool";
  get_attribute_value_bool(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_db(const std::string& name,double& value)
{
  attribute_list[e][name] = "double_db";
  get_attribute_value_db(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_dbspl(const std::string& name,double& value)
{
  attribute_list[e][name] = "double_dbspl";
  get_attribute_value_dbspl(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_db_float(const std::string& name,float& value)
{
  attribute_list[e][name] = "float_db";
  get_attribute_value_db_float(e,name,value);
}

void TASCAR::xml_element_t::get_attribute_deg(const std::string& name,double& value)
{
  attribute_list[e][name] = "double_degree";
  get_attribute_value_deg(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,TASCAR::pos_t& value)
{
  attribute_list[e][name] = "pos";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,std::vector<TASCAR::pos_t>& value)
{
  attribute_list[e][name] = "vector<pos>";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,std::vector<std::string>& value)
{
  attribute_list[e][name] = "vector<string>";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,std::vector<double>& value)
{
  attribute_list[e][name] = "vector<double>";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,std::vector<float>& value)
{
  attribute_list[e][name] = "vector<float>";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,std::vector<int32_t>& value)
{
  attribute_list[e][name] = "vector<int32>";
  get_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,TASCAR::levelmeter_t::weight_t& value)
{
  attribute_list[e][name] = "levelmeterweight";
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

void TASCAR::xml_element_t::set_attribute_dbspl(const std::string& name,double value)
{
  ::set_attribute_dbspl(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const std::string& value)
{
  e->set_attribute(name,value);
}

void TASCAR::xml_element_t::set_attribute_deg(const std::string& name,double value)
{
  ::set_attribute_double(e,name,value*RAD2DEG);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,double value)
{
  set_attribute_double(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,uint32_t value)
{
  set_attribute_uint32(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,int32_t value)
{
  set_attribute_int32(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,uint64_t value)
{
  set_attribute_uint64(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,int64_t value)
{
  set_attribute_int64(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const TASCAR::pos_t& value)
{
  set_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const std::vector<TASCAR::pos_t>& value)
{
  set_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const std::vector<std::string>& value)
{
  set_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const std::vector<double>& value)
{
  set_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const std::vector<int32_t>& value)
{
  set_attribute_value(e,name,value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,const TASCAR::levelmeter_t::weight_t& value)
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

void set_attribute_uint32(xmlpp::Element* elem,const std::string& name,uint32_t value)
{
  char ctmp[1024];
  sprintf(ctmp,"%d",value);
  elem->set_attribute(name,ctmp);
}

void set_attribute_int32(xmlpp::Element* elem,const std::string& name,int32_t value)
{
  char ctmp[1024];
  sprintf(ctmp,"%d",value);
  elem->set_attribute(name,ctmp);
}

void set_attribute_uint64(xmlpp::Element* elem,const std::string& name,uint64_t value)
{
  char ctmp[1024];
  sprintf(ctmp,"%ld",value);
  elem->set_attribute(name,ctmp);
}

void set_attribute_int64(xmlpp::Element* elem,const std::string& name,int64_t value)
{
  char ctmp[1024];
  sprintf(ctmp,"%ld",value);
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

void set_attribute_dbspl(xmlpp::Element* elem,const std::string& name,double value)
{
  char ctmp[1024];
  sprintf(ctmp,"%1.12g",20.0*log10(value/2e-5));
  elem->set_attribute(name,ctmp);
}

void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::pos_t& value)
{
  elem->set_attribute(name,value.print_cart(" "));
}

void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::levelmeter_t::weight_t& value)
{
  switch( value ){
  case TASCAR::levelmeter_t::Z :
    elem->set_attribute(name,"Z");
    break;
  }
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

void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<std::string>& value)
{
  std::string s;
  for(std::vector<std::string>::const_iterator i_vert=value.begin();i_vert!=value.end();++i_vert){
    if( i_vert != value.begin() )
      s += " ";
    s += *i_vert;
  }
  elem->set_attribute(name,s);
}


void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<double>& value)
{
  std::stringstream s;
  for(std::vector<double>::const_iterator i_vert=value.begin();i_vert!=value.end();++i_vert){
    if( i_vert != value.begin() )
      s << " ";
    s << *i_vert;
  }
  elem->set_attribute(name,s.str());
}

void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<int>& value)
{
  std::stringstream s;
  for(std::vector<int>::const_iterator i_vert=value.begin();i_vert!=value.end();++i_vert){
    if( i_vert != value.begin() )
      s << " ";
    s << *i_vert;
  }
  elem->set_attribute(name,s.str());
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,float& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  float tmpv(strtof(attv.c_str(),&c));
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
  if( !s.empty() ){
    std::stringstream ptxt(s);
    while( ptxt.good() ){
      TASCAR::pos_t p;
      ptxt >> p.x;
      if( ptxt.good() ){
        ptxt >> p.y;
        if( ptxt.good() ){
          ptxt >> p.z;
          value.push_back(p);
        }
      }
    }
  }
  return value;
}

std::vector<double> TASCAR::str2vecdouble(const std::string& s)
{
  std::vector<double> value;
  if( !s.empty() ){
    std::stringstream ptxt(s);
    while( ptxt.good() ){
      double p;
      ptxt >> p;
      value.push_back(p);
    }
  }
  return value;
}

std::vector<float> TASCAR::str2vecfloat(const std::string& s)
{
  std::vector<float> value;
  if( !s.empty() ){
    std::stringstream ptxt(s);
    while( ptxt.good() ){
      float p;
      ptxt >> p;
      value.push_back(p);
    }
  }
  return value;
}

std::vector<int32_t> TASCAR::str2vecint(const std::string& s)
{
  std::vector<int32_t> value;
  if( !s.empty() ){
    std::stringstream ptxt(s);
    while( ptxt.good() ){
      double p;
      ptxt >> p;
      value.push_back(p);
    }
  }
  return value;
}

std::vector<std::string> TASCAR::str2vecstr(const std::string& s)
{
  std::vector<std::string> value;
  if( !s.empty() ){
    std::stringstream ptxt(s);
    while( ptxt.good() ){
      std::string p;
      ptxt >> p;
      value.push_back(p);
    }
  }
  return value;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,TASCAR::levelmeter_t::weight_t& value)
{
  std::string svalue(elem->get_attribute_value(name));
  if( svalue.size() == 0 )
    return;
  if( svalue.size() != 1 )
    throw TASCAR::ErrMsg(std::string("invalid attribute value \"")+svalue+std::string("\" for attribute \"")+name+std::string("\"."));
  switch( toupper(svalue[0]) ){
  case 'Z':
    value = TASCAR::levelmeter_t::Z;
    break;
  default:
    throw TASCAR::ErrMsg(std::string("Unsupported weight type \"")+svalue+std::string("\" for attribute \"")+name+std::string("\"."));
  }
}


void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<TASCAR::pos_t>& value)
{
  value = TASCAR::str2vecpos(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<std::string>& value)
{
  value = TASCAR::str2vecstr(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<double>& value)
{
  value = TASCAR::str2vecdouble(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<float>& value)
{
  value = TASCAR::str2vecfloat(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<int32_t>& value)
{
  value = TASCAR::str2vecint(elem->get_attribute_value(name));
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

void get_attribute_value_dbspl(xmlpp::Element* elem,const std::string& name,double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = pow(10.0,0.05*tmpv)*2e-5;
}

void get_attribute_value_db_float(xmlpp::Element* elem,const std::string& name,float& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(),&c));
  if( c != attv.c_str() )
    value = pow(10.0,0.05*tmpv);
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,uint32_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long unsigned int tmpv(strtoul(attv.c_str(),&c,10));
  if( c != attv.c_str() )
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,uint64_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long unsigned int tmpv(strtoul(attv.c_str(),&c,10));
  if( c != attv.c_str() )
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,int32_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long int tmpv(strtol(attv.c_str(),&c,10));
  if( c != attv.c_str() )
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,int64_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long int tmpv(strtol(attv.c_str(),&c,10));
  if( c != attv.c_str() )
    value = tmpv;
}

void get_attribute_value_bool(xmlpp::Element* elem,const std::string& name,bool& value)
{
  std::string attv(elem->get_attribute_value(name));
  if( attv.size() )
    value = (attv == "true");
}

std::vector<std::string> TASCAR::xml_element_t::get_unused_attributes() const
{
  std::vector<std::string> retv;
  const xmlpp::Element::AttributeList al(e->get_attributes());
  for(xmlpp::Element::AttributeList::const_iterator il=al.begin();il!=al.end();++il){
    if( attribute_list[e].find((*il)->get_name())==attribute_list[e].end() )
      retv.push_back((*il)->get_name());
  }
  return retv;
}

void TASCAR::xml_element_t::validate_attributes(std::string& msg) const
{
  std::vector<std::string> unused(get_unused_attributes());
  if( unused.size() ){
    if( !msg.empty() )
      msg += "\n";
    char cline[256];
    sprintf(cline,"%d",e->get_line());
    msg += "Invalid attributes in element \""+e->get_name()+"\" (Line "+cline+"):";
    for(std::vector<std::string>::const_iterator it=unused.begin();it!=unused.end();++it)
      msg += " " + *it;
    msg += " (valid attributes are";
    for( std::map<std::string,std::string>::const_iterator 
           it=attribute_list[e].begin();
         it!=attribute_list[e].end();++it){
      if( it != attribute_list[e].begin() )
        msg += ",";
      msg += " " + it->first;
    }
    msg += ").";
  }
}


TASCAR::xml_doc_t::xml_doc_t()
  : doc(NULL)
{
  doc = new xmlpp::Document();
  doc->create_root_node("session");
}

TASCAR::xml_doc_t::xml_doc_t(const std::string& filename_or_data,load_type_t t)
  : doc(NULL)
{
  switch( t ){
  case LOAD_FILE :
    domp.parse_file(TASCAR::env_expand(filename_or_data));
    break;
  case LOAD_STRING :
    domp.parse_memory(filename_or_data);
    break;
  }
  doc = domp.get_document();
  if( !doc )
    throw TASCAR::ErrMsg("Unable to parse document.");
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
