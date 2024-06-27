/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
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

  protected:
    void add_scene(tsccfg::node_t e);
    void add_range(tsccfg::node_t);
    void add_connection(tsccfg::node_t);

  public:
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
  root.GET_ATTRIBUTE(srv_addr, "",
                     "OSC multicast address in case of UDP transport");
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
    const char* options = "hglv";
    struct option long_options[] = {{"help", 0, 0, 'h'},
                                    {"gendoc", 0, 0, 'g'},
                                    {"latex", 0, 0, 'l'},
                                    {"verbose", 0, 0, 'v'},
                                    {0, 0, 0, 0}};
    int opt(0);
    int option_index(0);
    bool gendoc(false);
    bool latex(false);
    bool verbose(false);
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
      case 'v':
        verbose = true;
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
    if(verbose)
      std::cout << "validating scene " << tscfile << std::endl;
    App::show_licenses_t c(tscfile);
    std::string v(c.show_unknown());
    if(v.size() && get_warnings().size())
      v += "\n";
    for(auto warn : get_warnings()) {
      v += "Warning: " + warn + "\n";
    }
    c.validate_attributes(v);
    if(gendoc)
      TASCAR::generate_plugin_documentation_tables(latex);
    std::cout << v << std::endl;
    if(!v.empty())
      return 1;
    if(verbose)
      std::cout << "Validation of \"" << tscfile << "\" was successful ("
                << c.scenes.size() << " scenes)" << std::endl;
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
