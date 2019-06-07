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
    void add_scene(xmlpp::Element *e);
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
  };

}

App::show_licenses_t::show_licenses_t(const std::string& session_filename)
  : session_core_t(session_filename,LOAD_FILE,session_filename)
{
  read_xml();
  GET_ATTRIBUTE(name);
  GET_ATTRIBUTE(srv_port);
  GET_ATTRIBUTE(srv_addr);
  GET_ATTRIBUTE(srv_proto);
}

void App::show_licenses_t::validate_attributes(std::string& msg) const
{
  TASCAR::tsc_reader_t::validate_attributes(msg);
  for(std::vector<TASCAR::render_core_t*>::const_iterator it=scenes.begin();it!=scenes.end();++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::range_t*>::const_iterator it=ranges.begin();it!=ranges.end();++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::connection_t*>::const_iterator it=connections.begin();
      it!=connections.end();++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::module_t*>::const_iterator it=modules.begin();it!=modules.end();++it)
    (*it)->validate_attributes(msg);
}

App::show_licenses_t::~show_licenses_t()
{
  for(std::vector<TASCAR::render_core_t*>::iterator sit=scenes.begin();sit!=scenes.end();++sit)
    delete (*sit);
}

void App::show_licenses_t::add_scene(xmlpp::Element* sne)
{
  scenes.push_back(new render_core_t(sne));
  scenes.back()->add_licenses( this );
}


void App::show_licenses_t::add_range(xmlpp::Element* src)
{
  if( !src )
    src = tsc_reader_t::e->add_child("range");
  ranges.push_back(new TASCAR::range_t(src));
}

void App::show_licenses_t::add_connection(xmlpp::Element* src)
{
  if( !src )
    src = tsc_reader_t::e->add_child("connect");
  connections.push_back(new TASCAR::connection_t(src));
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_gui -c configfile [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  std::string tscfile("");
  const char *options = "h";
  struct option long_options[] = { 
    { "help",     0, 0, 'h' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'h':
      usage(long_options);
      return -1;
    }
  }
  if( optind < argc )
    tscfile = argv[optind++];
  if( tscfile.size() == 0 ){
    usage(long_options);
    return -1;
  }
  App::show_licenses_t c(tscfile);
  std::string v(c.show_unknown());
  if( v.size() && warnings.size() )
    v+="\n";
  for(std::vector<std::string>::const_iterator it=warnings.begin();it!=warnings.end();++it){
    v+= "Warning: " + *it + "\n";
  }
  c.validate_attributes(v);
  std::cout << v << std::endl;
  if( !v.empty() )
    return 1;
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
