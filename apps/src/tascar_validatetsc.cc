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
    void add_scene(xmlpp::Element* e);
    void add_range(xmlpp::Element*);
    void add_connection(xmlpp::Element*);

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
  GET_ATTRIBUTE(srv_port, "", "OSC port number");
  GET_ATTRIBUTE(srv_addr, "", "OSC multicast address in case of UDP transport");
  GET_ATTRIBUTE(srv_proto, "", "OSC protocol, UDP or TCP");
  GET_ATTRIBUTE(name, "", "session name");
  GET_ATTRIBUTE(starturl, "", "URL of start page for display");
  add_licenses(this);
}

void App::show_licenses_t::validate_attributes(std::string& msg) const
{
  TASCAR::tsc_reader_t::validate_attributes(msg);
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

void App::show_licenses_t::add_scene(xmlpp::Element* sne)
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

void App::show_licenses_t::add_range(xmlpp::Element* src)
{
  if(!src)
    src = tsc_reader_t::e->add_child("range");
  ranges.push_back(new TASCAR::range_t(src));
}

void App::show_licenses_t::add_connection(xmlpp::Element* src)
{
  if(!src)
    src = tsc_reader_t::e->add_child("connect");
  connections.push_back(new TASCAR::connection_t(src));
}

std::string tolatex(std::string s)
{
  s = TASCAR::strrep(s, "_", "\\_");
  return s;
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
  std::map<std::string, std::map<std::string, cfg_var_desc_t>> attribute_list;
  for(auto elem : TASCAR::attribute_list) {
    for(auto attr : elem.second)
      attribute_list[elem.first->get_name()][attr.first] = attr.second;
  }
  for(auto elem : attribute_list) {
    for(auto attr : elem.second)
      types.insert(attr.second.type);
    if(latex) {
      std::string fname("tab" + elem.first + ".tex");
      std::cout << "Creating latex table for " + elem.first + " in file " +
                       fname + "."
                << std::endl;
      std::ofstream fh(fname);
      fh << "\\begin{snugshade}\n";
      fh << "\\begin{tabularx}{\\textwidth}{lXl}\n\\multicolumn{3}{l}{"
            "Attributes of element {\\bf " +
                tolatex(elem.first) + "}}\\\\\n";
      fh << "name & description (type, unit) & def.\\\\\\hline\n";
      for(auto attr : elem.second) {
        //  \indattr{name} & type & def & unit & Name of session (default:
        //  ``tascar'')
        fh << "\\hline\n";
        fh << "\\indattr{" << tolatex(attr.first) << "} & "
           << tolatex(attr.second.info) << " (" << tolatex(attr.second.type);
        if(!attr.second.unit.empty())
          fh << ", " << attr.second.unit;
        fh << ") ";
        fh << "&" << tolatex(attr.second.defaultval) << "\\\\" << std::endl;
      }
      fh << "\\hline\n\\end{tabularx}\n";
      fh << "\\end{snugshade}\n";
    } else {
      std::cout << elem.first << ":\n";
      for(auto attr : elem.second) {
        std::cout << "  " << attr.first << " (" << attr.second.type << ") ["
                  << attr.second.defaultval << attr.second.unit << "] "
                  << attr.second.info << std::endl;
      }
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
    if(v.size() && warnings.size())
      v += "\n";
    for(std::vector<std::string>::const_iterator it = warnings.begin();
        it != warnings.end(); ++it) {
      v += "Warning: " + *it + "\n";
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
