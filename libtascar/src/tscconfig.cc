#include "tscconfig.h"
#include "errorhandling.h"
#include <functional>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef USEXERCESXML

class xml_init_t {
public:
  xml_init_t() { xercesc::XMLPlatformUtils::Initialize(); };
  ~xml_init_t() { xercesc::XMLPlatformUtils::Terminate(); };
};

static xml_init_t xercesc_init;

std::basic_string<XMLCh> str2wstr(const char* text)
{
  XMLCh* resarr(xercesc::XMLString::transcode(text));
  std::basic_string<XMLCh> result(resarr);
  xercesc::XMLString::release(&resarr);
  return result;
}

std::basic_string<XMLCh> str2wstr(const std::string& text)
{
  XMLCh* resarr(xercesc::XMLString::transcode(text.c_str()));
  std::basic_string<XMLCh> result(resarr);
  xercesc::XMLString::release(&resarr);
  return result;
}

std::string wstr2str(const XMLCh* text)
{
  char* resarr(xercesc::XMLString::transcode(text));
  std::string result(resarr);
  xercesc::XMLString::release(&resarr);
  return result;
}
#endif

namespace TASCAR {

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

  static std::map<std::string, cfg_node_desc_t> attribute_list;
  static std::vector<std::string> warnings;
  static globalconfig_t config_;
  static size_t maxid(0);
} // namespace TASCAR

double TASCAR::config(const std::string& v, double d)
{
  return TASCAR::config_(v, d);
}

std::string TASCAR::config(const std::string& v, const std::string& d)
{
  return TASCAR::config_(v, d);
}

void TASCAR::config_forceoverwrite(const std::string& v, const std::string& d)
{
  TASCAR::config_.forceoverwrite(v, d);
}

std::map<std::string, TASCAR::cfg_node_desc_t>& TASCAR::get_attribute_list()
{
  return TASCAR::attribute_list;
}

std::vector<std::string>& TASCAR::get_warnings()
{
  return TASCAR::warnings;
}

std::string TASCAR::get_tuid()
{
  // todo: make thread safe
  char c[1024];
  sprintf(c, "%lx", ++maxid);
  return c;
}

std::vector<tsccfg::node_t> tsccfg::node_get_children(tsccfg::node_t& node,
                                                      const std::string& name)
{
  TASCAR_ASSERT(node);
  std::vector<tsccfg::node_t> children;
#ifdef USEPUGIXML
  for(auto sn : node.children()) {
    if(name.empty() || (name == tsccfg::node_get_name(sn)))
      children.push_back(sn);
  }
#elif defined(USEXERCESXML)
  auto nodelist(node->getChildNodes());
  for(size_t k = 0; k < nodelist->getLength(); ++k) {
    auto node(nodelist->item(k));
    if(node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      tsccfg::node_t sne(dynamic_cast<tsccfg::node_t>(node));
      if(sne)
        if(name.empty() || (name == tsccfg::node_get_name(sne)))
          children.push_back(sne);
    }
  }
#else
  for(auto sn : node->get_children()) {
    tsccfg::node_t sne(dynamic_cast<tsccfg::node_t>(sn));
    if(sne)
      if(name.empty() || (name == tsccfg::node_get_name(sne)))
        children.push_back(sne);
  }
#endif
  return children;
}

tsccfg::node_t tsccfg::node_add_child(tsccfg::node_t& node,
                                      const std::string& name)
{
  TASCAR_ASSERT(node);
#ifdef USEPUGIXML
  return node.append_child(name.c_str());
#elif defined(USEXERCESXML)
  return dynamic_cast<tsccfg::node_t>(node->appendChild(
      node->getOwnerDocument()->createElement(str2wstr(name).c_str())));
#else
  return node->add_child(name);
#endif
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

void node_register_attr(tsccfg::node_t& e, const std::string& name,
                        const std::string& value, const std::string& unit,
                        const std::string& info, const std::string& type)
{
  TASCAR_ASSERT(e);
  std::string path(tsccfg::node_get_path(e));
  TASCAR::attribute_list[path].category = tsccfg::node_get_name(e);
  TASCAR::attribute_list[path].type =
      tsccfg::node_get_attribute_value(e, "type");
  TASCAR::attribute_list[path].vars[name] = {name, type, value, unit, info};
}

void tsccfg::node_get_and_register_attribute(tsccfg::node_t& e,
                                             const std::string& name,
                                             std::string& value,
                                             const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, value, "", info, "string");
  if(tsccfg::node_has_attribute(e, name))
    value = tsccfg::node_get_attribute_value(e, name);
  else
    tsccfg::node_set_attribute(e, name, value);
}

size_t TASCAR::xml_element_t::hash(const std::vector<std::string>& attributes,
                                   bool test_children) const
{
  std::string v;
  for(auto attr : attributes)
    v += tsccfg::node_get_attribute_value(e, attr);
  if(test_children)
    for(auto sne : tsccfg::node_get_children(e))
      for(auto attr : attributes)
        v += tsccfg::node_get_attribute_value(sne, attr);
  std::hash<std::string> hash_fn;
  return hash_fn(v);
}

std::string TASCAR::to_string(const TASCAR::pos_t& x)
{
  return TASCAR::to_string(x.x) + " " + TASCAR::to_string(x.y) + " " +
         TASCAR::to_string(x.z);
}

std::string TASCAR::to_string(const std::vector<TASCAR::pos_t>& x)
{
  size_t k(0);
  std::string rv;
  for(auto p : x) {
    if(k)
      rv += " ";
    rv += TASCAR::to_string(p);
  }
  return rv;
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

std::string TASCAR::to_string(const std::vector<double>& value)
{
  std::stringstream s;
  for(std::vector<double>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  return s.str();
}

std::string TASCAR::to_string(const std::vector<float>& value)
{
  std::stringstream s;
  for(std::vector<float>::const_iterator i_vert = value.begin();
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
      readconfig("", doc.root());
    }
  }
  catch(const std::exception& e) {
  }
}

#ifdef USEPUGIXML
void del_whitespace(tsccfg::node_t node) {}
#elif defined(USEXERCESXML)
void del_whitespace(tsccfg::node_t node) {}
#else
void del_whitespace(xmlpp::Node* node)
{
  xmlpp::TextNode* nodeText = dynamic_cast<xmlpp::TextNode*>(node);
  if(nodeText && nodeText->is_white_space()) {
    nodeText->get_parent()->remove_child(node);
  } else {
    tsccfg::node_t nodeElement = dynamic_cast<tsccfg::node_t>(node);
    if(nodeElement) {
      xmlpp::Node::NodeList children = nodeElement->get_children();
      for(xmlpp::Node::NodeList::iterator nita = children.begin();
          nita != children.end(); ++nita) {
        del_whitespace(*nita);
      }
    }
  }
}
#endif

void TASCAR::xml_doc_t::save(const std::string& filename)
{
  if(doc) {
    del_whitespace(root());
#ifdef USEPUGIXML
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    doc.save_file(filename.c_str(), "  ", pugi::format_default,
                  pugi::encoding_utf8);
#elif defined(USEXERCESXML)
    auto serial(doc->getImplementation()->createLSSerializer());
    auto config(serial->getDomConfig());
    config->setParameter(str2wstr("format-pretty-print").c_str(), true);
    xercesc::LocalFileFormatTarget target(str2wstr(filename).c_str());
    xercesc::DOMLSOutput* pDomLsOutput(
        doc->getImplementation()->createLSOutput());
    pDomLsOutput->setByteStream(&target);
    serial->write(doc, pDomLsOutput);
    delete pDomLsOutput;
    delete serial;
#else
    doc->write_to_file_formatted(filename);
#endif
  }
}

std::string TASCAR::xml_doc_t::save_to_string()
{
  if(doc) {
    del_whitespace(root());
#ifdef USEPUGIXML
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    std::stringstream out;
    doc.save(out, "  ", pugi::format_default, pugi::encoding_utf8);
    return out.str();
#elif defined(USEXERCESXML)
    auto serial(doc->getImplementation()->createLSSerializer());
    auto config(serial->getDomConfig());
    config->setParameter(str2wstr("format-pretty-print").c_str(), true);
    xercesc::MemBufFormatTarget target;
    xercesc::DOMLSOutput* pDomLsOutput(
        doc->getImplementation()->createLSOutput());
    pDomLsOutput->setByteStream(&target);
    serial->write(doc, pDomLsOutput);
    std::string retv((char*)target.getRawBuffer());
    delete pDomLsOutput;
    delete serial;
    return retv;
#else
    return doc->write_to_string_formatted();
#endif
  }
  return "";
}

void TASCAR::globalconfig_t::forceoverwrite(const std::string& a,
                                            const std::string& b)
{
  cfg[a] = b;
}

void TASCAR::globalconfig_t::readconfig(const std::string& prefix,
                                        tsccfg::node_t& e)
{
  TASCAR_ASSERT(e);
  std::string key(prefix);
  if(prefix.size())
    key += ".";
  key += tsccfg::node_get_name(e);
  TASCAR::xml_element_t xe(e);
  if(xe.has_attribute("data")) {
    cfg[key] = tsccfg::node_get_attribute_value(e, "data");
  }
  for(auto sn : tsccfg::node_get_children(e)) {
    readconfig(key, sn);
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

void TASCAR::add_warning(std::string msg, const tsccfg::node_t& e)
{
  add_warning(msg + " (" + tsccfg::node_get_path(e) + ")");
}

void TASCAR::add_warning(std::string msg)
{
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

TASCAR::xml_element_t::xml_element_t(const tsccfg::node_t& src) : e(src)
{
  if(!e)
    throw TASCAR::ErrMsg("Invalid NULL element pointer (xml_element_t).");
}

TASCAR::xml_element_t::xml_element_t() : e(NULL) {}

TASCAR::xml_element_t::~xml_element_t() {}

bool tsccfg::node_has_attribute(const tsccfg::node_t& e,
                                const std::string& name)
{
  TASCAR_ASSERT(e);
#ifdef USEPUGIXML
  for(auto attr : e.attributes())
    if(attr.name() == name)
      return true;
  return false;
#elif defined(USEXERCESXML)
  return e->getAttributeNode(str2wstr(name).c_str());
#else
  for(auto attr : e->get_attributes())
    if(attr->get_name() == name)
      return true;
  return false;
#endif
}

std::vector<std::string> TASCAR::xml_element_t::get_attributes() const
{
  std::vector<std::string> r;
#ifdef USEPUGIXML
  for(auto a : e.attributes())
    r.push_back(a.name());
#elif defined(USEXERCESXML)
  auto attrs(e->getAttributes());
  for(size_t k = 0; k < attrs->getLength(); ++k)
    r.push_back(wstr2str(attrs->item(k)->getNodeName()));
#else
  for(auto a : e->get_attributes())
    r.push_back(a->get_name());
#endif
  return r;
}

bool TASCAR::xml_element_t::has_attribute(const std::string& name) const
{
  TASCAR_ASSERT(e);
  return tsccfg::node_has_attribute(e, name);
}

tsccfg::node_t TASCAR::xml_element_t::find_or_add_child(const std::string& name)
{
  TASCAR_ASSERT(e);
  for(auto ch : tsccfg::node_get_children(e))
    if(tsccfg::node_get_name(ch) == name)
      return ch;
  return add_child(name);
}

tsccfg::node_t TASCAR::xml_element_t::add_child(const std::string& name)
{
  TASCAR_ASSERT(e);
  return tsccfg::node_add_child(e, name);
}

std::string TASCAR::xml_element_t::get_attribute(const std::string& name)
{
  TASCAR_ASSERT(e);
  return tsccfg::node_get_attribute_value(e, name);
}

std::vector<tsccfg::node_t>
TASCAR::xml_element_t::get_children(const std::string& name)
{
  TASCAR_ASSERT(e);
  return tsccfg::node_get_children(e, name);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          std::string& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, value, unit, info, "string");
  if(has_attribute(name))
    value = tsccfg::node_get_attribute_value(e, name);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          double& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), unit, info, "double");
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name, float& value,
                                          const std::string& unit,
                                          const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), unit, info, "float");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, std::to_string(value), unit, info, "uint32");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, std::to_string(value), unit, info, "int32");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, std::to_string(value), unit, info, "uint64");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, std::to_string(value), unit, info, "int64");
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute_bits(const std::string& name,
                                               uint32_t& value,
                                               const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, to_string_bits(value), "", info, "bits32");
  if(has_attribute(name)) {
    std::string strval;
    strval = tsccfg::node_get_attribute_value(e, name);
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string_bool(value), unit, info,
                     "bool");
  if(has_attribute(name))
    get_attribute_value_bool(e, name, value);
  else
    set_attribute_bool(name, value);
}

void TASCAR::xml_element_t::get_attribute_db(const std::string& name,
                                             double& value,
                                             const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string_db(value), "dB", info,
                     "double");
  if(has_attribute(name))
    get_attribute_value_db(e, name, value);
  else
    set_attribute_db(name, value);
}

void TASCAR::xml_element_t::get_attribute_dbspl(const std::string& name,
                                                double& value,
                                                const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string_dbspl(value), "dB SPL", info,
                     "double");
  if(has_attribute(name))
    get_attribute_value_dbspl(e, name, value);
  else
    set_attribute_dbspl(name, value);
}

void TASCAR::xml_element_t::get_attribute_dbspl(const std::string& name,
                                                float& value,
                                                const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string_dbspl(value), "dB SPL", info,
                     "float");
  if(has_attribute(name))
    get_attribute_value_dbspl_float(e, name, value);
  else
    set_attribute_dbspl(name, value);
}

void TASCAR::xml_element_t::get_attribute_db(const std::string& name,
                                             float& value,
                                             const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string_db(value), "dB", info, "float");
  if(has_attribute(name))
    get_attribute_value_db_float(e, name, value);
  else
    set_attribute_db(name, value);
}

void TASCAR::xml_element_t::get_attribute_deg(const std::string& name,
                                              double& value,
                                              const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value * RAD2DEG), "deg", info,
                     "double");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), unit, info, "pos");
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          TASCAR::zyx_euler_t& value,
                                          const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string_deg(value), "deg", info,
                     "Euler rot");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), unit, info,
                     "pos array");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::vecstr2str(value), unit, info,
                     "string array");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), unit, info,
                     "double array");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), unit, info,
                     "float array");
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
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), unit, info,
                     "int32 array");
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::get_attribute(const std::string& name,
                                          TASCAR::levelmeter::weight_t& value,
                                          const std::string& info)
{
  TASCAR_ASSERT(e);
  node_register_attr(e, name, TASCAR::to_string(value), "", info, "f-weight");
  if(has_attribute(name))
    get_attribute_value(e, name, value);
  else
    set_attribute(name, value);
}

void TASCAR::xml_element_t::set_attribute_bool(const std::string& name,
                                               bool value)
{
  TASCAR_ASSERT(e);
  ::set_attribute_bool(e, name, value);
}

void TASCAR::xml_element_t::set_attribute_db(const std::string& name,
                                             double value)
{
  TASCAR_ASSERT(e);
  ::set_attribute_db(e, name, value);
}

void TASCAR::xml_element_t::set_attribute_dbspl(const std::string& name,
                                                double value)
{
  TASCAR_ASSERT(e);
  ::set_attribute_dbspl(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::string& value)
{
  TASCAR_ASSERT(e);
  tsccfg::node_set_attribute(e, name, value);
}

void TASCAR::xml_element_t::set_attribute_deg(const std::string& name,
                                              double value)
{
  TASCAR_ASSERT(e);
  ::set_attribute_double(e, name, value * RAD2DEG);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name, double value)
{
  TASCAR_ASSERT(e);
  set_attribute_double(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          uint32_t value)
{
  TASCAR_ASSERT(e);
  set_attribute_uint32(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          int32_t value)
{
  TASCAR_ASSERT(e);
  set_attribute_int32(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          uint64_t value)
{
  TASCAR_ASSERT(e);
  set_attribute_uint64(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          int64_t value)
{
  TASCAR_ASSERT(e);
  set_attribute_int64(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const TASCAR::pos_t& value)
{
  TASCAR_ASSERT(e);
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const TASCAR::zyx_euler_t& value)
{
  TASCAR_ASSERT(e);
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(
    const std::string& name, const std::vector<TASCAR::pos_t>& value)
{
  TASCAR_ASSERT(e);
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<std::string>& value)
{
  TASCAR_ASSERT(e);
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<double>& value)
{
  TASCAR_ASSERT(e);
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<float>& value)
{
  TASCAR_ASSERT(e);
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(const std::string& name,
                                          const std::vector<int32_t>& value)
{
  TASCAR_ASSERT(e);
  set_attribute_value(e, name, value);
}

void TASCAR::xml_element_t::set_attribute(
    const std::string& name, const TASCAR::levelmeter::weight_t& value)
{
  TASCAR_ASSERT(e);
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
  TASCAR_ASSERT(e);
  tsccfg::node_set_attribute(e, name, TASCAR::to_string_bits(value));
}

void set_attribute_uint32(tsccfg::node_t& elem, const std::string& name,
                          uint32_t value)
{
  TASCAR_ASSERT(elem);
  tsccfg::node_set_attribute(elem, name, std::to_string(value));
}

void set_attribute_int32(tsccfg::node_t& elem, const std::string& name,
                         int32_t value)
{
  TASCAR_ASSERT(elem);
  char ctmp[1024];
  sprintf(ctmp, "%d", value);
  tsccfg::node_set_attribute(elem, name, ctmp);
}

void set_attribute_uint64(tsccfg::node_t& elem, const std::string& name,
                          uint64_t value)
{
  TASCAR_ASSERT(elem);
  tsccfg::node_set_attribute(elem, name, std::to_string(value));
}

void set_attribute_int64(tsccfg::node_t& elem, const std::string& name,
                         int64_t value)
{
  TASCAR_ASSERT(elem);
  tsccfg::node_set_attribute(elem, name, std::to_string(value));
}

void set_attribute_bool(tsccfg::node_t& elem, const std::string& name,
                        bool value)
{
  TASCAR_ASSERT(elem);
  if(value)
    tsccfg::node_set_attribute(elem, name, "true");
  else
    tsccfg::node_set_attribute(elem, name, "false");
}

void set_attribute_double(tsccfg::node_t& elem, const std::string& name,
                          double value)
{
  TASCAR_ASSERT(elem);
  char ctmp[1024];
  sprintf(ctmp, "%1.12g", value);
  tsccfg::node_set_attribute(elem, name, ctmp);
}

void set_attribute_db(tsccfg::node_t& elem, const std::string& name,
                      double value)
{
  TASCAR_ASSERT(elem);
  char ctmp[1024];
  sprintf(ctmp, "%1.12g", 20.0 * log10(value));
  tsccfg::node_set_attribute(elem, name, ctmp);
}

void set_attribute_dbspl(tsccfg::node_t& elem, const std::string& name,
                         double value)
{
  TASCAR_ASSERT(elem);
  char ctmp[1024];
  sprintf(ctmp, "%1.12g", 20.0 * log10(value / 2e-5));
  tsccfg::node_set_attribute(elem, name, ctmp);
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const TASCAR::pos_t& value)
{
  TASCAR_ASSERT(elem);
  tsccfg::node_set_attribute(elem, name, value.print_cart(" "));
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const TASCAR::zyx_euler_t& value)
{
  TASCAR_ASSERT(elem);
  char ctmp[1024];
  sprintf(ctmp, "%1.12g %1.12g %1.12g", value.z * RAD2DEG, value.y * RAD2DEG,
          value.x * RAD2DEG);
  tsccfg::node_set_attribute(elem, name, ctmp);
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const TASCAR::levelmeter::weight_t& value)
{
  TASCAR_ASSERT(elem);
  tsccfg::node_set_attribute(elem, name, TASCAR::to_string(value));
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<TASCAR::pos_t>& value)
{
  TASCAR_ASSERT(elem);
  std::string s;
  for(std::vector<TASCAR::pos_t>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s += " ";
    s += i_vert->print_cart(" ");
  }
  tsccfg::node_set_attribute(elem, name, s);
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<std::string>& value)
{
  TASCAR_ASSERT(elem);
  std::string s;
  for(std::vector<std::string>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s += " ";
    s += *i_vert;
  }
  tsccfg::node_set_attribute(elem, name, s);
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<double>& value)
{
  TASCAR_ASSERT(elem);
  std::stringstream s;
  for(std::vector<double>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  tsccfg::node_set_attribute(elem, name, s.str());
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<float>& value)
{
  TASCAR_ASSERT(elem);
  std::stringstream s;
  for(std::vector<float>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  tsccfg::node_set_attribute(elem, name, s.str());
}

void set_attribute_value(tsccfg::node_t& elem, const std::string& name,
                         const std::vector<int>& value)
{
  TASCAR_ASSERT(elem);
  std::stringstream s;
  for(std::vector<int>::const_iterator i_vert = value.begin();
      i_vert != value.end(); ++i_vert) {
    if(i_vert != value.begin())
      s << " ";
    s << *i_vert;
  }
  tsccfg::node_set_attribute(elem, name, s.str());
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::string& value)
{
  TASCAR_ASSERT(elem);
  if(tsccfg::node_has_attribute(elem, name))
    value = tsccfg::node_get_attribute_value(elem, name);
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         double& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         float& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  float tmpv(strtof(attv.c_str(), &c));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         TASCAR::pos_t& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  TASCAR::pos_t tmpv;
  if(sscanf(attv.c_str(), "%lf%lf%lf", &(tmpv.x), &(tmpv.y), &(tmpv.z)) == 3) {
    value = tmpv;
  }
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         TASCAR::zyx_euler_t& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
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

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         TASCAR::levelmeter::weight_t& value)
{
  TASCAR_ASSERT(elem);
  std::string svalue(tsccfg::node_get_attribute_value(elem, name));
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

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<TASCAR::pos_t>& value)
{
  TASCAR_ASSERT(elem);
  value = TASCAR::str2vecpos(tsccfg::node_get_attribute_value(elem, name));
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<std::string>& value)
{
  TASCAR_ASSERT(elem);
  value = TASCAR::str2vecstr(tsccfg::node_get_attribute_value(elem, name));
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<double>& value)
{
  TASCAR_ASSERT(elem);
  value = TASCAR::str2vecdouble(tsccfg::node_get_attribute_value(elem, name));
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<float>& value)
{
  TASCAR_ASSERT(elem);
  value = TASCAR::str2vecfloat(tsccfg::node_get_attribute_value(elem, name));
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         std::vector<int32_t>& value)
{
  TASCAR_ASSERT(elem);
  value = TASCAR::str2vecint(tsccfg::node_get_attribute_value(elem, name));
}

void get_attribute_value_deg(const tsccfg::node_t& elem,
                             const std::string& name, double& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = DEG2RAD * tmpv;
}

void get_attribute_value_db(const tsccfg::node_t& elem, const std::string& name,
                            double& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = pow(10.0, 0.05 * tmpv);
}

void get_attribute_value_dbspl(const tsccfg::node_t& elem,
                               const std::string& name, double& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = pow(10.0, 0.05 * tmpv) * 2e-5;
}

void get_attribute_value_dbspl_float(const tsccfg::node_t& elem,
                                     const std::string& name, float& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  float tmpv(strtof(attv.c_str(), &c));
  if(c != attv.c_str())
    value = powf(10.0f, 0.05f * tmpv) * 2e-5f;
}

void get_attribute_value_db_float(const tsccfg::node_t& elem,
                                  const std::string& name, float& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  double tmpv(strtod(attv.c_str(), &c));
  if(c != attv.c_str())
    value = pow(10.0, 0.05 * tmpv);
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         uint32_t& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  long unsigned int tmpv(strtoul(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         uint64_t& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  long unsigned int tmpv(strtoul(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         int32_t& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  long int tmpv(strtol(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value(const tsccfg::node_t& elem, const std::string& name,
                         int64_t& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  char* c;
  long int tmpv(strtol(attv.c_str(), &c, 10));
  if(c != attv.c_str())
    value = tmpv;
}

void get_attribute_value_bool(const tsccfg::node_t& elem,
                              const std::string& name, bool& value)
{
  TASCAR_ASSERT(elem);
  std::string attv(tsccfg::node_get_attribute_value(elem, name));
  if(attv.size())
    value = (attv == "true");
}

std::vector<std::string> TASCAR::xml_element_t::get_unused_attributes() const
{
  TASCAR_ASSERT(e);
  std::vector<std::string> retv;
  std::string path(tsccfg::node_get_path(e));
  for(auto attrname : get_attributes()) {
    if(attribute_list[path].vars.find(attrname) ==
       attribute_list[path].vars.end())
      retv.push_back(attrname);
  }
  return retv;
}

void TASCAR::xml_element_t::validate_attributes(std::string& msg) const
{
  TASCAR_ASSERT(e);
  std::vector<std::string> unused(get_unused_attributes());
  if(unused.size()) {
    if(!msg.empty())
      msg += "\n";
    std::string path(tsccfg::node_get_path(e));
    msg += "Invalid attributes in element \"" + tsccfg::node_get_name(e) +
           "\" (path " + path + "):";
    for(auto attr : unused)
      msg += " " + attr;
    msg += " (valid attributes are:";
    for(auto attr : attribute_list[path].vars)
      msg += " " + attr.first;
    msg += ").";
  }
}

#ifdef USEPUGIXML
TASCAR::xml_doc_t::xml_doc_t()
{
  doc.load_string("<?xml version=1.0?>\n<session/>");
  root = get_root_node();
}
#elif defined(USEXERCESXML)
TASCAR::xml_doc_t::xml_doc_t() : doc(NULL)
{
  xercesc::DOMImplementation* impl(
      xercesc::DOMImplementationRegistry::getDOMImplementation(
          str2wstr("XML 1.0").c_str()));
  TASCAR_ASSERT(impl);
  doc = impl->createDocument(0, str2wstr("session").c_str(), NULL);
  root = get_root_node();
}
#else
TASCAR::xml_doc_t::xml_doc_t() : doc(NULL), freedoc(true)
{
  doc = new xmlpp::Document();
  doc->create_root_node("session");
  root = get_root_node();
}
#endif

#ifdef USEPUGIXML
TASCAR::xml_doc_t::xml_doc_t(const std::string& filename_or_data, load_type_t t)
{
  pugi::xml_parse_result result;
  switch(t) {
  case LOAD_FILE:
    result = doc.load_file(TASCAR::env_expand(filename_or_data).c_str());
    if(!result)
      throw TASCAR::ErrMsg(
          "Error while parsing XML: " + std::string(result.description()) +
          " at " + TASCAR::env_expand(filename_or_data) + " line " +
          std::to_string(result.offset));
    break;
  case LOAD_STRING:
    result = doc.load_string(filename_or_data.c_str());
    if(!result)
      throw TASCAR::ErrMsg(
          "Error while parsing XML: " + std::string(result.description()) +
          " at line " + std::to_string(result.offset));
    break;
  }
  root = get_root_node();
}
#elif defined(USEXERCESXML)
TASCAR::xml_doc_t::xml_doc_t(const std::string& filename_or_data, load_type_t t)
    : doc(NULL)
{
  std::string msg;
  domp.setValidationScheme(xercesc::XercesDOMParser::Val_Never);
  domp.setDoNamespaces(false);
  domp.setDoSchema(false);
  domp.setLoadExternalDTD(false);
  switch(t) {
  case LOAD_FILE:
    msg = "parsing file \"" + filename_or_data + "\"";
    domp.parse(filename_or_data.c_str());
    break;
  case LOAD_STRING:
    msg = "parsing string of " + std::to_string(filename_or_data.size()) +
          " characters";
    xercesc::MemBufInputSource myxml_buf((XMLByte*)(filename_or_data.c_str()),
                                         filename_or_data.size(),
                                         "xml_doc_t(in memory)");
    domp.parse(myxml_buf);
    break;
  }
  doc = domp.getDocument();
  if(!doc)
    throw TASCAR::ErrMsg("Unable to parse document (" + msg + ").");
  if(!get_root_node())
    throw TASCAR::ErrMsg("The document has no root node (" + msg + ").");
  root = get_root_node();
}
#else
TASCAR::xml_doc_t::xml_doc_t(const std::string& filename_or_data, load_type_t t)
    : doc(NULL), freedoc(false)
{
  std::string msg;
  switch(t) {
  case LOAD_FILE:
    domp.parse_file(TASCAR::env_expand(filename_or_data));
    msg = "parsing file \"" + filename_or_data + "\"";
    break;
  case LOAD_STRING:
    domp.parse_memory(filename_or_data);
    msg = "parsing string of " + std::to_string(filename_or_data.size()) +
          " characters";
    break;
  }
  doc = domp.get_document();
  if(!doc)
    throw TASCAR::ErrMsg("Unable to parse document (" + msg + ").");
  root = doc->get_root_node();
  if(!root)
    throw TASCAR::ErrMsg("The document has no root node (" + msg + ").");
}
#endif

#ifdef USEPUGIXML
TASCAR::xml_doc_t::xml_doc_t(const tsccfg::node_t& src)
{
  doc.append_copy(src);
  root = doc.document_element();
}
#elif defined(USEXERCESXML)
TASCAR::xml_doc_t::xml_doc_t(const tsccfg::node_t& src) : doc(NULL)
{
  domp.setValidationScheme(xercesc::XercesDOMParser::Val_Never);
  domp.setDoNamespaces(false);
  domp.setDoSchema(false);
  domp.setLoadExternalDTD(false);
  xercesc::DOMImplementation* impl(
      xercesc::DOMImplementationRegistry::getDOMImplementation(
          str2wstr("XML 1.0").c_str()));
  TASCAR_ASSERT(impl);
  doc = impl->createDocument(0, str2wstr("session").c_str(), NULL);
  auto impnode(doc->importNode(src, true));
  doc->replaceChild(impnode, get_root_node());
  root = get_root_node();
}
#else
TASCAR::xml_doc_t::xml_doc_t(const tsccfg::node_t& src)
    : doc(new xmlpp::Document()), freedoc(true)
{
  doc->create_root_node_by_import(src);
  root = doc->get_root_node();
}
#endif

TASCAR::xml_doc_t::~xml_doc_t()
{
#ifdef USEPUGIXML
  // do nothing
#elif defined(USEXERCESXML)
  // cleanup document
#else
  if(freedoc && doc)
    delete doc;
#endif
}

tsccfg::node_t TASCAR::xml_doc_t::get_root_node()
{
  TASCAR_ASSERT(doc);
#ifdef USEPUGIXML
  DEBUG(1);
  return doc.document_element();
#elif defined(USEXERCESXML)
  return doc->getDocumentElement();
#else
  return doc->get_root_node();
#endif
}

std::string TASCAR::xml_element_t::get_element_name() const
{
  TASCAR_ASSERT(e);
  return tsccfg::node_get_name(e);
}

std::string tsccfg::node_get_text(tsccfg::node_t& n, const std::string& child)
{
  TASCAR_ASSERT(n);
#ifdef USEPUGIXML
  return n.text().get();
#elif defined(USEXERCESXML)
  if(!n)
    return "";
  if(child.empty())
    return wstr2str(n->getTextContent());
  std::string retval;
  for(auto ch : tsccfg::node_get_children(n, child))
    retval += tsccfg::node_get_text(ch);
  return retval;
#else
  if(n) {
    if(child.size()) {
      xmlpp::Node::NodeList ch = n->get_children(child);
      if(ch.size()) {
        xmlpp::NodeSet stxt = (*ch.begin())->find("text()");
        xmlpp::TextNode* txt = dynamic_cast<xmlpp::TextNode*>(*(stxt.begin()));
        if(txt) {
          return txt->get_content();
        }
      }
    } else {
      xmlpp::NodeSet stxt = n->find("text()");
      if(stxt.begin() != stxt.end()) {
        xmlpp::TextNode* txt = dynamic_cast<xmlpp::TextNode*>(*(stxt.begin()));
        if(txt) {
          return txt->get_content();
        }
      }
    }
  }
#endif
  return "";
}

void tsccfg::node_set_text(tsccfg::node_t& node, const std::string& text)
{
#ifdef USEPUGIXML
  node.remove_children();
  node.text().set(text.c_str());
#elif defined(USEXERCESXML)
  node->setTextContent(str2wstr(text).c_str());
#else
  for(auto sn : node->get_children())
    node->remove_child(sn);
  node->add_child_text(text);
#endif
}

void tsccfg::node_import_node(tsccfg::node_t& node, const tsccfg::node_t& src)
{
#ifdef USEPUGIXML
  node.append_copy(src);
#elif defined(USEXERCESXML)
  auto impnode(node->getOwnerDocument()->importNode(src, true));
  node->appendChild(impnode);
#else
  node->import_node(src);
#endif
}

std::string tsccfg::node_get_attribute_value(const tsccfg::node_t& node,
                                             const std::string& name)
{
  TASCAR_ASSERT(node);
#ifdef USEPUGIXML
  return node.attribute(name.c_str()).value();
#elif defined(USEXERCESXML)
  return wstr2str(node->getAttribute(str2wstr(name).c_str()));
#else
  return node->get_attribute_value(name);
#endif
}

std::string tsccfg::node_get_name(const tsccfg::node_t& node)
{
  TASCAR_ASSERT(node);
#ifdef USEPUGIXML
  return node.name();
#elif defined(USEXERCESXML)
  return wstr2str(node->getTagName());
#else
  return node->get_name();
#endif
}

void tsccfg::node_set_name(const tsccfg::node_t& node, const std::string& name)
{
  TASCAR_ASSERT(node);
#ifdef USEPUGIXML
  TASCAR_ASSERT(false);
#elif defined(USEXERCESXML)
  node->getOwnerDocument()->renameNode(node, NULL, str2wstr(name).c_str());
#else
  TASCAR_ASSERT(false);
#endif
}

std::string tsccfg::node_get_path(const tsccfg::node_t& node)
{
  TASCAR_ASSERT(node);
#ifdef USEPUGIXML
  tsccfg::node_t sib(node);
  size_t sib_prev(0);
  while((sib = sib.previous_sibling(node.name())))
    sib_prev++;
  sib = node;
  size_t sib_next(0);
  while((sib = sib.next_sibling(node.name())))
    sib_next++;
  if(sib_prev + sib_next == 0)
    return node.path();
  return std::string(node.path()) + "[" + std::to_string(sib_prev + 1) + "]";
#elif defined(USEXERCESXML)
  std::string thisname(tsccfg::node_get_name(node));
  xercesc::DOMNode* sib(node);
  size_t sib_prev(0);
  while((sib = sib->getPreviousSibling())) {
    tsccfg::node_t sibe(dynamic_cast<tsccfg::node_t>(sib));
    if(sibe && (tsccfg::node_get_name(sibe) == thisname))
      sib_prev++;
  }
  sib = node;
  size_t sib_next(0);
  while((sib = sib->getNextSibling())) {
    tsccfg::node_t sibe(dynamic_cast<tsccfg::node_t>(sib));
    if(sibe && (tsccfg::node_get_name(sibe) == thisname))
      sib_next++;
  }
  std::string nodepath(std::string("/") + thisname);
  if(sib_prev + sib_next != 0)
    nodepath += "[" + std::to_string(sib_prev + 1) + "]";
  tsccfg::node_t parent(dynamic_cast<tsccfg::node_t>(node->getParentNode()));
  if(parent)
    nodepath = tsccfg::node_get_path(parent) + nodepath;
  return nodepath;
#else
  return node->get_path();
#endif
}

void tsccfg::node_set_attribute(tsccfg::node_t& node, const std::string& name,
                                const std::string& value)
{
  TASCAR_ASSERT(node);
#ifdef USEPUGIXML
  node.attribute(name.c_str()).set_value(value.c_str());
#elif defined(USEXERCESXML)
  node->setAttribute(str2wstr(name).c_str(), str2wstr(value).c_str());
#else
  node->set_attribute(name, value);
#endif
}

const std::vector<tsccfg::node_t>
tsccfg::node_get_children(const tsccfg::node_t& node)
{
  TASCAR_ASSERT(node);
#ifdef USEPUGIXML
  TASCAR_ASSERT(false);
  return std::vector<tsccfg::node_t>();
#elif defined(USEXERCESXML)
  TASCAR_ASSERT(false);
  return std::vector<tsccfg::node_t>();
#else
  std::vector<tsccfg::node_t> retval;
  for(auto sn : node->get_children()) {
    tsccfg::node_t sne(dynamic_cast<tsccfg::node_t>(sn));
    if(sne)
      retval.push_back(sne);
  }
  return retval;
#endif
}

// double tsccfg::node_xpath_to_number(tsccfg::node_t& node,
//                                    const std::string& path)
//{
//  TASCAR_ASSERT(node);
//#ifdef USEPUGIXML
//  pugi::xpath_query query(path.c_str());
//  return query.evaluate_number(node);
//#elif defined(USEXERCESXML)
//  try {
//    // auto res(node->getOwnerDocument()->evaluate(
//    //    str2wstr(path).c_str(), node, NULL,
//    //    xercesc::DOMXPathResult::NUMBER_TYPE, NULL));
//    auto res(node->getOwnerDocument()->evaluate(
//        str2wstr(path).c_str(), node, NULL,
//        xercesc::DOMXPathResult::FIRST_ORDERED_NODE_TYPE, NULL));
//    if(!res)
//      throw TASCAR::ErrMsg("xpath evaluation failed");
//    auto resnode(res->getNodeValue());
//    if(!resnode)
//      throw TASCAR::ErrMsg("No node found matching " + path);
//    DEBUG(wstr2str(resnode->getNodeValue()));
//    double val(-1);
//    DEBUG(1);
//    delete res;
//    return val;
//  }
//  catch(const xercesc::DOMXPathException& e) {
//    DEBUG(wstr2str(e.getMessage()));
//    throw TASCAR::ErrMsg("DOMXPathException while evaluating xpath " + path +
//                         " for node " + tsccfg::node_get_path(node) + ": " +
//                         wstr2str(e.getMessage()));
//  }
//  catch(const xercesc::DOMException&) {
//    DEBUG("DOMException");
//    throw;
//  }
//  DEBUG(1);
//#else
//  return node->eval_to_number(path);
//#endif
//}

TASCAR::cfg_node_desc_t::cfg_node_desc_t() {}
TASCAR::cfg_node_desc_t::~cfg_node_desc_t() {}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
