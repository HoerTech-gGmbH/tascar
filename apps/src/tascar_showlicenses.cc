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
#include <getopt.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

namespace App {

  class show_licenses_t : public TASCAR::tsc_reader_t {
  public:
    show_licenses_t(const std::string& session_filename);
    ~show_licenses_t();

  protected:
    virtual void add_scene(tsccfg::node_t e);

  private:
    std::vector<TASCAR::render_core_t*> scenes;
  };

} // namespace App

App::show_licenses_t::show_licenses_t(const std::string& session_filename)
    : tsc_reader_t(session_filename, LOAD_FILE, session_filename)
{
  read_xml();
}

App::show_licenses_t::~show_licenses_t()
{
  for(std::vector<TASCAR::render_core_t*>::iterator sit = scenes.begin();
      sit != scenes.end(); ++sit)
    delete(*sit);
}

void App::show_licenses_t::add_scene(tsccfg::node_t sne)
{
  scenes.push_back(new render_core_t(sne));
  scenes.back()->add_licenses(this);
}

void usage(struct option* opt)
{
  std::cout << "Usage:\n\ntascar_showlicenses -c sessionfile "
               "[options]\n\nOptions:\n\n";
  while(opt->name) {
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg ? "#" : "")
              << "\n  --" << opt->name << (opt->has_arg ? "=#" : "") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  std::string tscfile("");
  const char* options = "h";
  struct option long_options[] = {{"help", 0, 0, 'h'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      usage(long_options);
      return 0;
    }
  }
  if(optind < argc)
    tscfile = argv[optind++];
  if(tscfile.size() == 0) {
    usage(long_options);
    return -1;
  }
  App::show_licenses_t c(tscfile);
  std::cout << c.legal_stuff();
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
