#ifndef XMLCONFIG_H
#define XMLCONFIG_H

#include "coordinates.h"
#include "levelmeter.h"

namespace TASCAR {

  std::string default_string(const std::string& src,const std::string& def);

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
    void get_attribute_dbspl(const std::string& name,double& value);
    void get_attribute_db_float(const std::string& name,float& value);
    void get_attribute_deg(const std::string& name,double& value);
    void get_attribute(const std::string& name,TASCAR::pos_t& value);
    void get_attribute(const std::string& name,TASCAR::levelmeter_t::weight_t& value);
    void get_attribute(const std::string& name,std::vector<TASCAR::pos_t>& value);
    void get_attribute(const std::string& name,std::vector<std::string>& value);
    void get_attribute(const std::string& name,std::vector<double>& value);
    void get_attribute(const std::string& name,std::vector<int32_t>& value);
    // set attributes:
    void set_attribute_bool(const std::string& name,bool value);
    void set_attribute_db(const std::string& name,double value);
    void set_attribute_dbspl(const std::string& name,double value);
    void set_attribute(const std::string& name,const std::string& value);
    void set_attribute(const std::string& name,double value);
    void set_attribute_deg(const std::string& name,double value);
    void set_attribute(const std::string& name,unsigned int value);
    void set_attribute(const std::string& name,const TASCAR::pos_t& value);
    void set_attribute(const std::string& name,const TASCAR::levelmeter_t::weight_t& value);
    void set_attribute(const std::string& name,const std::vector<TASCAR::pos_t>& value);
    void set_attribute(const std::string& name,const std::vector<std::string>& value);
    void set_attribute(const std::string& name,const std::vector<double>& value);
    void set_attribute(const std::string& name,const std::vector<int32_t>& value);
    virtual void write_xml() = 0;
    xmlpp::Element* find_or_add_child(const std::string& name);
    xmlpp::Element* e;
  protected:
    // book keeping
  };

  class scene_node_base_t : public xml_element_t {
  public:
    scene_node_base_t(xmlpp::Element*);
    virtual ~scene_node_base_t();
    virtual void prepare(double fs, uint32_t fragsize) = 0;
  };

  std::string env_expand( std::string s );

  std::vector<TASCAR::pos_t> str2vecpos(const std::string& s);
  std::vector<std::string> str2vecstr(const std::string& s);
  std::vector<double> str2vecdouble(const std::string& s);
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

}

void get_attribute_value(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,unsigned int& value);
void get_attribute_value_bool(xmlpp::Element* elem,const std::string& name,bool& value);
void get_attribute_value_db(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value_dbspl(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value_db_float(xmlpp::Element* elem,const std::string& name,float& value);
void get_attribute_value_deg(xmlpp::Element* elem,const std::string& name,double& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,TASCAR::pos_t& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<TASCAR::pos_t>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<std::string>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<double>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,std::vector<int32_t>& value);
void get_attribute_value(xmlpp::Element* elem,const std::string& name,TASCAR::levelmeter_t::weight_t& value);

void set_attribute_bool(xmlpp::Element* elem,const std::string& name,bool value);
void set_attribute_db(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_dbspl(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_double(xmlpp::Element* elem,const std::string& name,double value);
void set_attribute_uint(xmlpp::Element* elem,const std::string& name,unsigned int value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::pos_t& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<TASCAR::pos_t>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<std::string>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<double>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const std::vector<int32_t>& value);
void set_attribute_value(xmlpp::Element* elem,const std::string& name,const TASCAR::levelmeter_t::weight_t& value);

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

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
