/**
   \section license License (GPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#include "session.h"
#include <fstream>
#include <getopt.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

namespace App {

  class show_licenses_t : public TASCAR::session_core_t {
  public:
    show_licenses_t(const std::string& session_filename);
    ~show_licenses_t();
    virtual void validate_attributes(std::string&) const;
    void show_doc(bool latex);

  protected:
    void add_scene(tsccfg::node_t e);
    void add_range(tsccfg::node_t);
    void add_connection(tsccfg::node_t);

  private:
    std::vector<TASCAR::render_core_t*> scenes;
    std::vector<TASCAR::range_t*> ranges;
    std::vector<TASCAR::connection_t*> connections;
    std::vector<TASCAR::module_t*> modules;
    std::string name;
    std::string srv_port;
    std::string srv_addr;
    std::string srv_proto;
    std::string starturl;
    std::set<std::string> namelist;
  };

} // namespace App

App::show_licenses_t::show_licenses_t(const std::string& session_filename)
    : session_core_t(session_filename, LOAD_FILE, session_filename),
      name("tascar"), srv_port("9877"), srv_proto("UDP") //,
// starturl("http://news.tascar.org/")
{
  read_xml();
  root.GET_ATTRIBUTE(srv_port, "", "OSC port number");
  root.GET_ATTRIBUTE(srv_addr, "", "OSC multicast address in case of UDP transport");
  root.GET_ATTRIBUTE(srv_proto, "", "OSC protocol, UDP or TCP");
  root.GET_ATTRIBUTE(name, "", "session name");
  root.GET_ATTRIBUTE(starturl, "", "URL of start page for display");
  add_licenses(this);
}

void App::show_licenses_t::validate_attributes(std::string& msg) const
{
  root.validate_attributes(msg);
  for(std::vector<TASCAR::render_core_t*>::const_iterator it = scenes.begin();
      it != scenes.end(); ++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::range_t*>::const_iterator it = ranges.begin();
      it != ranges.end(); ++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::connection_t*>::const_iterator it =
          connections.begin();
      it != connections.end(); ++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::module_t*>::const_iterator it = modules.begin();
      it != modules.end(); ++it)
    (*it)->validate_attributes(msg);
}

App::show_licenses_t::~show_licenses_t()
{
  for(std::vector<TASCAR::render_core_t*>::iterator sit = scenes.begin();
      sit != scenes.end(); ++sit)
    delete(*sit);
}

void App::show_licenses_t::add_scene(tsccfg::node_t sne)
{
  TASCAR::render_core_t* newscene(NULL);
  try {
    newscene = new render_core_t(sne);
    if(namelist.find(newscene->name) != namelist.end())
      throw TASCAR::ErrMsg("A scene of name \"" + newscene->name +
                           "\" already exists in the session.");
    namelist.insert(newscene->name);
    scenes.push_back(newscene);
    scenes.back()->add_licenses(this);
  }
  catch(...) {
    delete newscene;
    throw;
  }
}

void App::show_licenses_t::add_range(tsccfg::node_t src)
{
  ranges.push_back(new TASCAR::range_t(src));
}

void App::show_licenses_t::add_connection(tsccfg::node_t src)
{
  connections.push_back(new TASCAR::connection_t(src));
}

std::string tolatex(std::string s)
{
  s = TASCAR::strrep(s, "_", "\\_");
  return s;
}

struct elem_cfg_var_desc_t {
  std::string elem;
  std::map<std::string, cfg_var_desc_t> attr;
  std::set<std::string> parents;
  std::string type;
};

void make_common(std::map<std::string, std::set<std::string>>& parentchildren,
                 std::map<std::string, elem_cfg_var_desc_t>& elems,
                 const std::set<std::string>& list,
                 const std::string& commonname)
{
  if(list.size()) {
    for(auto ch : list)
      parentchildren[commonname].insert(ch);
    // first get a list of common attributes:
    std::set<std::string> common_attributes;
    for(auto attr : elems[(*(list.begin()))].attr)
      common_attributes.insert(attr.first);
    for(auto elem : list) {
      std::set<std::string> attrs;
      for(auto attr : elems[elem].attr)
        attrs.insert(attr.first);
      std::set<std::string> rmattrs;
      for(auto attr : common_attributes)
        if(attrs.find(attr) == attrs.end())
          rmattrs.insert(attr);
      for(auto attr : rmattrs)
        common_attributes.erase(attr);
    }
    // attribute map for common object:
    std::map<std::string, cfg_var_desc_t> commonattr;
    for(auto attr : common_attributes) {
      // fill common attributes from first element in list:
      commonattr[attr] = elems[(*(list.begin()))].attr[attr];
      // remote common attributes from list:
      for(auto elem : list) {
        elems[elem].attr.erase(attr);
        elems[elem].parents.insert(commonname);
      }
    }
    elems[commonname] = {commonname, commonattr};
  }
}

void App::show_licenses_t::show_doc(bool latex)
{
  std::map<std::string, std::string> tdesc;
  tdesc["float"] = "single precision floating point number";
  tdesc["double"] = "double precision floating point number";
  tdesc["int32"] = "signed 32 bit integer number";
  tdesc["uint32"] = "unsigned 32 bit integer number";
  tdesc["bool"] = "boolean, \"true\" or \"false\"";
  tdesc["string"] = "character array";
  tdesc["f-weight"] =
      "frequency weighting, one of \"Z\", \"A\", \"C\" or \"bandpass\"";
  tdesc["bits32"] = "list of numbers 0 to 31, or \"all\"";
  tdesc["pos"] =
      "Cartesian coordinates, triplet of doubles separated by spaces";
  tdesc["string array"] = "space separated array of strings";
  std::set<std::string> types;
  std::map<std::string, elem_cfg_var_desc_t> attribute_list;
  std::map<std::string, std::set<std::string>> categories;
  for(auto elem : TASCAR::get_attribute_list() ) {
    std::string category(elem.second.category);
    std::string cattype(elem.second.type);
    std::map<std::string, cfg_var_desc_t> previousattr(
        attribute_list[category + cattype].attr);
    for(auto attr : elem.second.vars)
      previousattr[attr.first] = attr.second;
    attribute_list[category + cattype] = {category, previousattr, {}, cattype};
    categories[category].insert(category + cattype);
  }
  for(auto cat : categories) {
    std::cout << "category " << cat.first << std::endl;
    for(auto type : cat.second) {
      std::cout << "  " << type << std::endl;
    }
  }
  std::map<std::string, std::set<std::string>> parentchildren;
  make_common(parentchildren, attribute_list, categories["receiver"],
              "receiver");
  make_common(parentchildren, attribute_list, categories["reverb"], "receiver");
  make_common(parentchildren, attribute_list, categories["sound"], "sound");
  make_common(parentchildren, attribute_list,
              {"receiver", "source", "diffuse", "facegroup", "face",
               "boundingbox", "obstacle", "mask"},
              "objects");
  make_common(parentchildren, attribute_list,
              {"receiver", "source", "diffuse", "facegroup", "face", "mask",
               "obstacle"},
              "routes");
  make_common(parentchildren, attribute_list, {"receiver", "sound", "diffuse"},
              "ports");
  for(auto elem : attribute_list) {
    for(auto attr : elem.second.attr)
      types.insert(attr.second.type);
    if(latex && (!elem.second.attr.empty())) {
      std::string fname("tab" + elem.first + ".tex");
      std::cout << "Creating latex table for " + elem.first + " in file " +
                       fname + "."
                << std::endl;
      std::ofstream fh(fname);
      fh << "\\begin{snugshade}\n";
      fh << "\\label{attrtab:" << elem.first << "}\n"
         << "Attributes of ";
      if(elem.second.type.empty())
        fh << "element {\\bf " << tolatex(elem.first) + "}";
      else
        fh << tolatex(elem.second.elem) << " element {\\bf "
           << tolatex(elem.second.type) + "}";
      if(!parentchildren[elem.first].empty()) {
        fh << " (";
        size_t k(0);
        for(auto child : parentchildren[elem.first]) {
          if(k)
            fh << " ";
          std::string xchild(child);
          if(attribute_list[child].type.size())
            xchild = attribute_list[child].type;
          fh << "{\\hyperref[attrtab:" << child << "]{" << tolatex(xchild)
             << "}}";
          ++k;
        }
        fh << ")";
      }
      if(!elem.second.parents.empty()) {
        fh << ", inheriting from";
        for(auto parent : elem.second.parents)
          fh << " \\hyperref[attrtab:" << parent << "]{{\\bf "
             << tolatex(parent) << "}}";
      }
      fh << "\n\n";
      fh << "\\begin{tabularx}{\\textwidth}{lXl}\n\\hline\n";
      fh << "name & description (type, unit) & def.\\\\\n\\hline\n";
      for(auto attr : elem.second.attr) {
        //  \indattr{name} & type & def & unit & Name of session (default:
        //  ``tascar'')
        fh << "\\hline\n";
        fh << "\\indattr{" << tolatex(attr.first) << "} & "
           << tolatex(attr.second.info) << " (" << tolatex(attr.second.type);
        if(!attr.second.unit.empty())
          fh << ", " << attr.second.unit;
        fh << ") ";
        fh << "& " << tolatex(attr.second.defaultval) << "\\\\" << std::endl;
      }
      fh << "\\hline\n\\end{tabularx}\n";
      fh << "\\end{snugshade}\n";
    }
    std::cout << elem.first << " - " << elem.second.type;
    if(!elem.second.parents.empty()) {
      std::cout << ", inheriting from";
      for(auto parent : elem.second.parents)
        std::cout << " " << parent;
    }
    std::cout << ":\n";
    for(auto attr : elem.second.attr) {
      std::cout << "  " << attr.first << " (" << attr.second.type << ") ["
                << attr.second.defaultval << attr.second.unit << "] "
                << attr.second.info << std::endl;
    }
  }
  std::cout << std::endl << "types:" << std::endl;
  for(auto t : types)
    std::cout << "  " << t << ": " << tdesc[t] << std::endl;
}

void usage(struct option* opt)
{
  std::cout << "Usage:\n\ntascar_validatetsc -c sessionfile "
               "[options]\n\nOptions:\n\n";
  while(opt->name) {
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg ? "#" : "")
              << "\n  --" << opt->name << (opt->has_arg ? "=#" : "") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  try {
    std::string tscfile("");
    const char* options = "hgl";
    struct option long_options[] = {{"help", 0, 0, 'h'},
                                    {"gendoc", 0, 0, 'g'},
                                    {"latex", 0, 0, 'l'},
                                    {0, 0, 0, 0}};
    int opt(0);
    int option_index(0);
    bool gendoc(false);
    bool latex(false);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case 'h':
        usage(long_options);
        return 0;
        break;
      case 'g':
        gendoc = true;
        break;
      case 'l':
        latex = true;
        break;
      }
    }
    if(optind < argc)
      tscfile = argv[optind++];
    if(tscfile.size() == 0) {
      usage(long_options);
      return -1;
    }
    App::show_licenses_t c(tscfile);
    std::string v(c.show_unknown());
    if(v.size() && get_warnings().size())
      v += "\n";
    for(auto warn : get_warnings()) {
      v += "Warning: " + warn + "\n";
    }
    c.validate_attributes(v);
    if(gendoc)
      c.show_doc(latex);
    std::cout << v << std::endl;
    if(!v.empty())
      return 1;
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 2;
  }
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
