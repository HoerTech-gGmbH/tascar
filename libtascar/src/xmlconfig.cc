#include "xmlconfig.h"
#include "errorhandling.h"
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace TASCAR {
  std::map<xmlpp::Element*, std::map<std::string, cfg_var_desc_t>>
      attribute_list;
  std::vector<std::string> warnings;
  globalconfig_t config;
  size_t maxid(0);
} // namespace TASCAR

std::string TASCAR::get_tuid()
{
  // todo: make thread safe
  char c[1024];
  sprintf(c, "%lx", ++maxid);
  return c;
}

std::string TASCAR::strrep(std::string s, const std::string& pat,
                           const std::string& rep)
{
  std::string out_string("");
  std::string::size_type len = pat.size();
  std::string::size_type pos;
  while((pos = s.find(pat)) < s.size()) {
    out_string += s.substr(0, pos);
    out_string += rep;
    s.erase(0, pos + len);
  }
  s = out_string + s;
  return s;
}

void TASCAR::xmlpp_get_and_register_attribute(xmlpp::Element* e,
                                              const std::string& name,
                                              std::string& value,
                                              const std::string& info)
{
  TASCAR::attribute_list[e][name] = {name, "string", value, info};
  if(TASCAR::xmlpp_has_attribute(e, name))
    value = e->get_attribute_value(name);
  else
    e->set_attribute(name, value);
}

size_t TASCAR::xml_element_t::hash(const std::vector<std::string>& attributes,
                                   bool test_children) const
{
  std::string v;
  for(auto it = attributes.begin(); it != attributes.end(); ++it)
    v += e->get_attribute_value(*it);
  if(test_children) {
    xmlpp::Node::NodeList subnodes(e->get_children());
    for(auto sn = subnodes.begin(); sn != subnodes.end(); ++sn) {
      xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
      if(sne)
        for(auto it = attributes.begin(); it != attributes.end(); ++it) {
          if(sne->get_attribute_value(*it).size()) {
            v += sne->get_attribute_value(*it);
          }
        }
    }
  }
  std::hash<std::string> hash_fn;
  return hash_fn(v);
}

std::string TASCAR::to_string(const TASCAR::pos_t& x)
{
  return TASCAR::to_string(x.x) + " " + TASCAR::to_string(x.y) + " " +
         TASCAR::to_string(x.z);
}

std::string TASCAR::to_string(const TASCAR::zyx_euler_t& x)
{
  return TASCAR::to_string(x.z) + " " + TASCAR::to_string(x.y) + " " +
         TASCAR::to_string(x.x);
}

std::string TASCAR::to_string_deg(const TASCAR::zyx_euler_t& x)
{
  return TASCAR::to_string(x.z * RAD2DEG) + " " +
         TASCAR::to_string(x.y * RAD2DEG) + " " +
         TASCAR::to_string(x.x * RAD2DEG);
}

std::string TASCAR::to_string(const TASCAR::levelmeter::weight_t& value)
{
  switch(value) {
  case TASCAR::levelmeter::Z:
    return "Z";
    break;
  case TASCAR::levelmeter::A:
    return "A";
    break;
  case TASCAR::levelmeter::C:
    return "C";
    break;
  case TASCAR::levelmeter::bandpass:
    return "bandpass";
    break;
  }
  return "";
}

std::string TASCAR::to_string(double x)
{
  char ctmp[1024];
  sprintf(ctmp, "%g", x);
  return ctmp;
}

std::string TASCAR::to_string(float x)
{
  char ctmp[1024];
  sprintf(ctmp, "%g", x);
  return ctmp;
}

std::string TASCAR::to_string(uint32_t x)
{
  return std::to_string(x);
}

std::string TASCAR::to_string(int32_t x)
{
  return std::to_string(x);
}

std::string TASCAR::to_string_bool(bool value)
{
  if(value)
    return "true";
  return "false";
}

std::string TASCAR::to_string(const std::vector<int>& value)
{
  std::stringstream s;
  for(std::vector<int>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  return s.str();
}

std::string TASCAR::days_to_string(double x)
{
  int d(floor(x));
  int h(floor(24 * (x - d)));
  char ctmp[1024];
  if(d == 1)
    sprintf(ctmp, "1 day %d hours", h);
  else
    sprintf(ctmp, "%d days %d hours", d, h);
  return ctmp;
}

std::string TASCAR::to_string_db(double value)
{
  char ctmp[1024];
  sprintf(ctmp, "%g", 20.0 * log10(value));
  return ctmp;
}

std::string TASCAR::to_string_dbspl(double value)
{
  char ctmp[1024];
  sprintf(ctmp, "%g", 20.0 * log10(value / 2e-5));
  return ctmp;
}

std::string TASCAR::to_string_db(float value)
{
  char ctmp[1024];
  sprintf(ctmp, "%g", 20.0f * log10f(value));
  return ctmp;
}

std::string TASCAR::to_string_dbspl(float value)
{
  char ctmp[1024];
  sprintf(ctmp, "%g", 20.0f * log10f(value / 2e-5f));
  return ctmp;
}

bool file_exists_ov(const std::string& fname)
{
  if(access(fname.c_str(), F_OK) != -1)
    return true;
  return false;
}

std::string localgetenv(const std::string& env)
{
  if(char* s = getenv(env.c_str()))
    return s;
  return "";
}

TASCAR::globalconfig_t::globalconfig_t()
{
  readconfig("/etc/tascar/defaults.xml");
  readconfig("${HOME}/.tascardefaults.xml");
}

void TASCAR::globalconfig_t::readconfig(const std::string& fname)
{
  try {
    std::string lfname(TASCAR::env_expand(fname));
    if(file_exists_ov(lfname)) {
      setlocale(LC_ALL, "C");
      xml_doc_t doc(lfname, xml_doc_t::LOAD_FILE);
      readconfig("", doc.doc->get_root_node());
    }
  }
  catch(const std::exception& e) {
  }
}

void TASCAR::globalconfig_t::forceoverwrite(const std::string& a,
                                            const std::string& b)
{
  cfg[a] = b;
}

void TASCAR::globalconfig_t::readconfig(const std::string& prefix,
                                        xmlpp::Element* e)
{
  std::string key(prefix);
  if(prefix.size())
    key += ".";
  key += e->get_name();
  TASCAR::xml_element_t xe(e);
  if(xe.has_attribute("data")) {
    cfg[key] = e->get_attribute_value("data");
  }
  xmlpp::Node::NodeList subnodes(e->get_children());
  for(auto sn = subnodes.begin(); sn != subnodes.end(); ++sn) {
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if(sne)
      readconfig(key, sne);
  }
}

double TASCAR::globalconfig_t::operator()(const std::string& key,
                                          double def) const
{
  if(localgetenv("TASCARSHOWGLOBAL").size())
    std::cout << key << " (" << def;
  auto e(cfg.find(key));
  if(e != cfg.end()) {
    if(localgetenv("TASCARSHOWGLOBAL").size())
      std::cout << "=>" << e->second.c_str() << ")\n";
    return atof(e->second.c_str());
  }
  if(localgetenv("TASCARSHOWGLOBAL").size())
    std::cout << ")\n";
  return def;
}

std::string TASCAR::globalconfig_t::operator()(const std::string& key,
                                               const std::string& def) const
{
  if(localgetenv("TASCARSHOWGLOBAL").size())
    std::cout << key << " (" << def << ")\n";
  auto e(cfg.find(key));
  if(e != cfg.end())
    return e->second;
  return def;
}

void TASCAR::add_warning(std::string msg, xmlpp::Element* e)
{
  if(e) {
    char ctmp[256];
    sprintf(ctmp, "Line %d: ", e->get_line());
    msg = ctmp + msg;
  }
  warnings.push_back(msg);
  std::cerr << "Warning: " << msg << std::endl;
}

std::string TASCAR::tscbasename(const std::string& s)
{
  return s.substr(s.rfind("/") + 1);
}

std::string TASCAR::default_string(const std::string& src,
                                   const std::string& def)
{
  if(src.empty())
    return def;
  return src;
}

TASCAR::xml_element_t::xml_element_t(xmlpp::Element* src) : e(src)
{
  if(!e)
    throw TASCAR::ErrMsg("Invalid NULL element pointer.");
}

TASCAR::xml_element_t::~xml_element_t() {}

bool TASCAR::xmlpp_has_attribute(xmlpp::Element* e, const std::string& name)
{
  const xmlpp::Element::AttributeList atts(e->get_attributes());
  for(xmlpp::Element::AttributeList::const_iterator it = atts.begin();
      it != atts.end(); ++it)
    if((*it)->get_name() == name)
      return true;
  return false;
}

std::vector<std::string> TASCAR::xml_element_t::get_attributes() const
{
  std::vector<std::string> r;
  const xmlpp::Element::AttributeList atts(e->get_attributes());
  for(auto a : atts)
    r.push_back(a->get_name());
  return r;
}

bool TASCAR::xml_element_t::has_attribute(const std::string& name) const
{
  return xmlpp_has_attribute(e, name);
}

xmlpp::Element*
TASCAR::xml_element_t::find_or_add_child(const std::string& name)
{
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn = subnodes.begin();
      sn != subnodes.end(); ++sn) {
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if(sne && (sne->get_name() == name))
      return sne;
  }
  return e->add_child(name);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          std::string& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "string", value, unit, info};
  if(has_attribute(name))
    value = e->get_attribute_value(name);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          double& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "double", TASCAR::to_string(value), unit,
                             info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name, float& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "float", TASCAR::to_string(value), unit,
                             info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          uint32_t& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "uint32", std::to_string(value), unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          int32_t& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "int32", std::to_string(value), unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          uint64_t& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "uint64", std::to_string(value), unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          int64_t& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "int64", std::to_string(value), unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute_bits(const std::string& name,
                                               uint32_t& value,
                                               const std::string& info)
{
  attribute_list[e][name] = {name, "bits32", to_string_bits(value), "", info};
  if(has_attribute(name)) {
    std::string strval;
    strval = e->get_attribute_value(name);
    if(strval == "all") {
      value = 0xffffffff;
    } else {
      std::vector<int32_t> bits(TASCAR::str2vecint(strval));
      value = 0;
      for(uint32_t k = 0; k < bits.size(); ++k) {
        if(bits[k] < 32)
          value |= (1 << bits[k]);
      }
    }
  } else {
    set_attribute_bits(name, value);
  }
}

void TASCAR::xml_element_t::get_attribute_bool(const std::string& name,
                                               bool& value,
                                               const std::string& unit,
                                               const std::string& info)
{
  attribute_list[e][name] = {name, "bool", TASCAR::to_string_bool(value), unit,
                             info};
  if(has_attribute(name))
    get_attribute_value_bool(e, name, value);
  else
    set_attribute_bool(name, value);
}

void TASCAR::xml_element_t::get_attribute_db(const std::string& name,
                                             double& value,
                                             const std::string& info)
{
  attribute_list[e][name] = {name, "double", TASCAR::to_string_db(value), "dB",
                             info};
  if(has_attribute(name))
    get_attribute_value_db(e, name, value);
  else
    set_attribute_db(name, value);
}

void TASCAR::xml_element_t::get_attribute_dbspl(const std::string& name,
                                                double& value,
                                                const std::string& info)
{
  attribute_list[e][name] = {name, "double", TASCAR::to_string_dbspl(value),
                             "dB SPL", info};
  if(has_attribute(name))
    get_attribute_value_dbspl(e, name, value);
  else
    set_attribute_dbspl(name, value);
}

void TASCAR::xml_element_t::get_attribute_dbspl(const std::string& name,
                                                float& value,
                                                const std::string& info)
{
  attribute_list[e][name] = {name, "float", TASCAR::to_string_dbspl(value),
                             "dB SPL", info};
  if(has_attribute(name))
    get_attribute_value_dbspl_float(e, name, value);
  else
    set_attribute_dbspl(name, value);
}

void TASCAR::xml_element_t::get_attribute_db(const std::string& name,
                                             float& value,
                                             const std::string& info)
{
  attribute_list[e][name] = {name, "float", TASCAR::to_string_db(value), "dB",
                             info};
  if(has_attribute(name))
    get_attribute_value_db_float(e, name, value);
  else
    set_attribute_db(name, value);
}

void TASCAR::xml_element_t::get_attribute_deg(const std::string& name,
                                              double& value,
                                              const std::string& info)
{
  attribute_list[e][name] = {name, "double", TASCAR::to_string(value * RAD2DEG),
                             "deg", info};
  if(has_attribute(name))
    get_attribute_value_deg(e, name, value);
  else
    set_attribute_deg(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          TASCAR::pos_t& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "pos", TASCAR::to_string(value), unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          TASCAR::zyx_euler_t& value,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "Euler rot", TASCAR::to_string_deg(value),
                             "deg", info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          std::vector<TASCAR::pos_t>& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "vector<pos>", "XXX", unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          std::vector<std::string>& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "string array", TASCAR::vecstr2str(value),
                             unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          std::vector<double>& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "vector<double>", "XXX", unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          std::vector<float>& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "vector<float>", "XXX", unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          std::vector<int32_t>& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "vector<int32>", TASCAR::to_string(value),
                             unit, info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          TASCAR::levelmeter::weight_t& value,
                                          const std::string& info)
{
  attribute_list[e][name] = {name, "f-weight", TASCAR::to_string(value), "",
                             info};
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::set_attribute_bool(const std::string& name,
                                               bool value)
{
  ::set_attribute_bool(e, name, value);
}

void TASCAR::xml_element_t::set_attribute_db(const std::string& name,
                                             double value)
{
  ::set_attribute_db(e, name, value);
}

void TASCAR::xml_element_t::set_attribute_dbspl(const std::string& name,
                                                double value)
{
  ::set_attribute_dbspl(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::string& value)
{
  e->set_attribute(name, value);
}

void TASCAR::xml_element_t::set_attribute_deg(const std::string& name,
                                              double value)
{
  ::set_attribute_double(e, name, value * RAD2DEG);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name, double value)
{
  set_attribute_double(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          uint32_t value)
{
  set_attribute_uint32(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          int32_t value)
{
  set_attribute_int32(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          uint64_t value)
{
  set_attribute_uint64(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          int64_t value)
{
  set_attribute_int64(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const TASCAR::pos_t& value)
{
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const TASCAR::zyx_euler_t& value)
{
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(
    const std::string& name, const std::vector<TASCAR::pos_t>& value)
{
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<std::string>& value)
{
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<double>& value)
{
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<float>& value)
{
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<int32_t>& value)
{
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(
    const std::string& name, const TASCAR::levelmeter::weight_t& value)
{
  set_attribute_value(e, name, value);
}

std::string TASCAR::env_expand(std::string s)
{
  size_t spos;
  while((spos = s.find("${")) != std::string::npos) {
    size_t epos(s.find("}", spos));
    if(epos == std::string::npos)
      epos = s.size();
    std::string env(s.substr(spos + 2, epos - spos - 2));
    s.replace(spos, epos - spos + 1, localgetenv(env));
  }
  return s;
}

std::string TASCAR::to_string_bits(uint32_t value)
{
  if(value == 0xffffffff)
    return "all";
  std::string sv;
  for(uint32_t b = 0; b < 32; ++b) {
    if(value & (1 << b))
      sv += std::to_string(b) + " ";
  }
  if(!sv.empty())
    sv.erase(sv.size() - 1, 1);
  return sv;
}

void TASCAR::xml_element_t::set_attribute_bits(const std::string& name,
                                               uint32_t value)
{
  e->set_attribute(name, TASCAR::to_string_bits(value));
}

void set_attribute_uint32(xmlpp::Element* elem, const std::string& name,
                          uint32_t value)
{
  elem->set_attribute(name, std::to_string(value));
}

void set_attribute_int32(xmlpp::Element* elem, const std::string& name,
                         int32_t value)
{
  char ctmp[1024];
  sprintf(ctmp, "%d", value);
  elem->set_attribute(name, ctmp);
}

void set_attribute_uint64(xmlpp::Element* elem, const std::string& name,
                          uint64_t value)
{
  elem->set_attribute(name, std::to_string(value));
}

void set_attribute_int64(xmlpp::Element* elem, const std::string& name,
                         int64_t value)
{
  elem->set_attribute(name, std::to_string(value));
}

void set_attribute_bool(xmlpp::Element* elem, const std::string& name,
                        bool value)
{
  if(value)
    elem->set_attribute(name, "true");
  else
    elem->set_attribute(name, "false");
}

void set_attribute_double(xmlpp::Element* elem, const std::string& name,
                          double value)
{
  char ctmp[1024];
  sprintf(ctmp, "%1.12g", value);
  elem->set_attribute(name, ctmp);
}

void set_attribute_db(xmlpp::Element* elem, const std::string& name,
                      double value)
{
  char ctmp[1024];
  sprintf(ctmp, "%1.12g", 20.0 * log10(value));
  elem->set_attribute(name, ctmp);
}

void set_attribute_dbspl(xmlpp::Element* elem, const std::string& name,
                         double value)
{
  char ctmp[1024];
  sprintf(ctmp, "%1.12g", 20.0 * log10(value / 2e-5));
  elem->set_attribute(name, ctmp);
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const TASCAR::pos_t& value)
{
  elem->set_attribute(name, value.print_cart(" "));
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const TASCAR::zyx_euler_t& value)
{
  char ctmp[1024];
  sprintf(ctmp, "%1.12g %1.12g %1.12g", value.z * RAD2DEG, value.y * RAD2DEG,
          value.x * RAD2DEG);
  elem->set_attribute(name, ctmp);
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const TASCAR::levelmeter::weight_t& value)
{
  elem->set_attribute(name, TASCAR::to_string(value));
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const std::vector<TASCAR::pos_t>& value)
{
  std::string s;
  for(std::vector<TASCAR::pos_t>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s += " ";
    s += i_vert->print_cart(" ");
  }
  elem->set_attribute(name, s);
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const std::vector<std::string>& value)
{
  std::string s;
  for(std::vector<std::string>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s += " ";
    s += *i_vert;
  }
  elem->set_attribute(name, s);
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const std::vector<double>& value)
{
  std::stringstream s;
  for(std::vector<double>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  elem->set_attribute(name, s.str());
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const std::vector<float>& value)
{
  std::stringstream s;
  for(std::vector<float>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  elem->set_attribute(name, s.str());
}

void set_attribute_value(xmlpp::Element* elem, const std::string& name,
                         const std::vector<int>& value)
{
  std::stringstream s;
  for(std::vector<int>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  elem->set_attribute(name, s.str());
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         float& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  float tmpv(strtof(attv.c_str(), &c));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         TASCAR::pos_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  TASCAR::pos_t tmpv;
  if(sscanf(attv.c_str(), "%lf%lf%lf", &(tmpv.x), &(tmpv.y), &(tmpv.z)) == 3) {
    value = tmpv;
  }
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         TASCAR::zyx_euler_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  TASCAR::zyx_euler_t tmpv;
  if(sscanf(attv.c_str(), "%lf%lf%lf", &(tmpv.z), &(tmpv.y), &(tmpv.x)) == 3) {
    tmpv.x *= DEG2RAD;
    tmpv.y *= DEG2RAD;
    tmpv.z *= DEG2RAD;
    value = tmpv;
  }
}

std::vector<TASCAR::pos_t> TASCAR::str2vecpos(const std::string& s)
{
  std::vector<TASCAR::pos_t> value;
  if(!s.empty()) {
    std::stringstream ptxt(s);
    while(ptxt.good()) {
      TASCAR::pos_t p;
      ptxt >> p.x;
      if(ptxt.good()) {
        ptxt >> p.y;
        if(ptxt.good()) {
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
  if(!s.empty()) {
    std::stringstream ptxt(s);
    while(ptxt.good()) {
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
  if(!s.empty()) {
    std::stringstream ptxt(s);
    while(ptxt.good()) {
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
  if(!s.empty()) {
    std::stringstream ptxt(s);
    while(ptxt.good()) {
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
  if(!s.empty()) {
    std::stringstream ptxt(s);
    while(ptxt.good()) {
      std::string p;
      ptxt >> p;
      value.push_back(p);
    }
  }
  return value;
}

std::string TASCAR::vecstr2str(const std::vector<std::string>& s)
{
  std::string rv;
  for(auto it = s.begin(); it != s.end(); ++it) {
    if(it != s.begin())
      rv += " ";
    rv += *it;
  }
  return rv;
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         TASCAR::levelmeter::weight_t& value)
{
  std::string svalue(elem->get_attribute_value(name));
  if(svalue.size() == 0)
    return;
  if(svalue == "Z")
    value = TASCAR::levelmeter::Z;
  else if(svalue == "C")
    value = TASCAR::levelmeter::C;
  else if(svalue == "A")
    value = TASCAR::levelmeter::A;
  else if(svalue == "bandpass")
    value = TASCAR::levelmeter::bandpass;
  else
    throw TASCAR::ErrMsg(std::string("Unsupported weight type \"") + svalue +
                         std::string("\" for attribute \"") + name +
                         std::string("\"."));
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         std::vector<TASCAR::pos_t>& value)
{
  value = TASCAR::str2vecpos(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         std::vector<std::string>& value)
{
  value = TASCAR::str2vecstr(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         std::vector<double>& value)
{
  value = TASCAR::str2vecdouble(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         std::vector<float>& value)
{
  value = TASCAR::str2vecfloat(elem->get_attribute_value(name));
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         std::vector<int32_t>& value)
{
  value = TASCAR::str2vecint(elem->get_attribute_value(name));
}

void get_attribute_value_deg(xmlpp::Element* elem, const std::string& name,
                             double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = DEG2RAD * tmpv;
}

void get_attribute_value_db(xmlpp::Element* elem, const std::string& name,
                            double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = pow(10.0, 0.05 * tmpv);
}

void get_attribute_value_dbspl(xmlpp::Element* elem, const std::string& name,
                               double& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = pow(10.0, 0.05 * tmpv) * 2e-5;
}

void get_attribute_value_dbspl_float(xmlpp::Element* elem,
                                     const std::string& name, float& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  float tmpv(strtof(attv.c_str(), &c));
  if(c != attv.c_str())
    value = powf(10.0f, 0.05f * tmpv) * 2e-5f;
}

void get_attribute_value_db_float(xmlpp::Element* elem, const std::string& name,
                                  float& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = pow(10.0, 0.05 * tmpv);
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         uint32_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long unsigned int tmpv(strtoul(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         uint64_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long unsigned int tmpv(strtoul(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         int32_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long int tmpv(strtol(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(xmlpp::Element* elem, const std::string& name,
                         int64_t& value)
{
  std::string attv(elem->get_attribute_value(name));
  char* c;
  long int tmpv(strtol(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value_bool(xmlpp::Element* elem, const std::string& name,
                              bool& value)
{
  std::string attv(elem->get_attribute_value(name));
  if(attv.size())
    value = (attv == "true");
}

std::vector<std::string> TASCAR::xml_element_t::get_unused_attributes() const
{
  std::vector<std::string> retv;
  const xmlpp::Element::AttributeList al(e->get_attributes());
  for(xmlpp::Element::AttributeList::const_iterator il = al.begin();
      il != al.end(); ++il) {
    if(attribute_list[e].find((*il)->get_name()) == attribute_list[e].end())
      retv.push_back((*il)->get_name());
  }
  return retv;
}

void TASCAR::xml_element_t::validate_attributes(std::string& msg) const
{
  std::vector<std::string> unused(get_unused_attributes());
  if(unused.size()) {
    if(!msg.empty())
      msg += "\n";
    char cline[256];
    sprintf(cline, "%d", e->get_line());
    msg += "Invalid attributes in element \"" + e->get_name() + "\" (Line " +
           cline + "):";
    for(auto attr : unused)
      msg += " " + attr;
    msg += " (valid attributes are:";
    for(auto attr : attribute_list[e])
      msg += " " + attr.first;
    msg += ").";
  }
}

TASCAR::xml_doc_t::xml_doc_t() : doc(NULL), freedoc(true)
{
  doc = new xmlpp::Document();
  doc->create_root_node("session");
}

TASCAR::xml_doc_t::xml_doc_t(const std::string& filename_or_data, load_type_t t)
    : doc(NULL), freedoc(false)
{
  switch(t) {
  case LOAD_FILE:
    domp.parse_file(TASCAR::env_expand(filename_or_data));
    break;
  case LOAD_STRING:
    domp.parse_memory(filename_or_data);
    break;
  }
  doc = domp.get_document();
  if(!doc)
    throw TASCAR::ErrMsg("Unable to parse document.");
}

TASCAR::xml_doc_t::~xml_doc_t()
{
  if(freedoc && doc)
    delete doc;
}

TASCAR::msg_t::msg_t(xmlpp::Element* e)
    : TASCAR::xml_element_t(e), msg(lo_message_new())
{
  GET_ATTRIBUTE(path, "", "OSC path name");
  for(auto sn : e->get_children()) {
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(sn));
    if(sne) {
      TASCAR::xml_element_t tsne(sne);
      if(sne->get_name() == "f") {
        double v(0);
        tsne.GET_ATTRIBUTE(v, "", "float value");
        lo_message_add_float(msg, v);
      }
      if(sne->get_name() == "i") {
        int32_t v(0);
        tsne.GET_ATTRIBUTE(v, "", "int value");
        lo_message_add_int32(msg, v);
      }
      if(sne->get_name() == "s") {
        std::string v("");
        tsne.GET_ATTRIBUTE(v, "", "string value");
        lo_message_add_string(msg, v.c_str());
      }
    }
  }
}

TASCAR::msg_t::~msg_t()
{
  lo_message_free(msg);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
