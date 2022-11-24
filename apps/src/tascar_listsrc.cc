/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2022 Giso Grimm
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

#include "tscconfig.h"
#include <unistd.h>
#include <getopt.h>
#include <set>
#include "tascar_os.h"
#include <libgen.h>

using namespace TASCAR;

void usage(struct option* opt)
{
  std::cout << "Usage:\n\ntascar_listsrc sessionfile "
               "[options]\n\nOptions:\n\n";
  while(opt->name) {
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg ? "#" : "")
              << "\n  --" << opt->name << (opt->has_arg ? "=#" : "") << "\n\n";
    opt++;
  }
}

std::set<std::string> sounds;
std::set<std::string> faces;
std::set<std::string> tracks;

void get_src(tsccfg::node_t n)
{
  if((tsccfg::node_get_name(n) == "sndfile") ||
     (tsccfg::node_get_name(n) == "sndfileasync")) {
    auto name = tsccfg::node_get_attribute_value(n, "name");
    if(!name.empty())
      sounds.insert(name);
  }
  if((tsccfg::node_get_name(n) == "facegroup") ||
     (tsccfg::node_get_name(n) == "obstacle")) {
    auto name = tsccfg::node_get_attribute_value(n, "importraw");
    if(!name.empty())
      faces.insert(name);
  }
  if(tsccfg::node_get_name(n) == "load") {
    auto name = tsccfg::node_get_attribute_value(n, "name");
    if(!name.empty())
      tracks.insert(name);
  }
  if(tsccfg::node_get_name(n) == "velocity") {
    auto name = tsccfg::node_get_attribute_value(n, "csvfile");
    if(!name.empty())
      tracks.insert(name);
  }
  auto children = tsccfg::node_get_children(n);
  for(auto child : children)
    get_src(child);
}

bool file_exists_lsrc(const std::string& fname)
{
  if(access(fname.c_str(), F_OK) != -1)
    return true;
  return false;
}

int main(int argc, char** argv)
{
  std::string tscfile("");
  const char* options = "hm";
  struct option long_options[] = {
      {"help", 0, 0, 'h'}, {"missing", 0, 0, 'm'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  bool showmissing(false);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      usage(long_options);
      return 0;
      break;
    case 'm':
      showmissing = true;
      break;
    }
  }
  if(optind < argc)
    tscfile = argv[optind++];
  if(tscfile.size() == 0) {
    usage(long_options);
    return -1;
  }
  xml_doc_t doc(tscfile, xml_doc_t::LOAD_FILE);
  get_src(doc.root.e);
  std::string session_path;
  {
    char c_fname[tscfile.size() + 1];
    char c_respath[PATH_MAX];
    memcpy(c_fname, tscfile.c_str(), tscfile.size() + 1);
    session_path = TASCAR::realpath(dirname(c_fname), c_respath);
    // Change current working directory of the complete process to the
    // directory containing the main configuration file in order to
    // resolve any relative paths present in the configuration.
    if(chdir(session_path.c_str()) != 0)
      add_warning("Unable to change directory.");
  }
  for(auto name : sounds)
    if(!showmissing || !file_exists_lsrc(name))
      std::cout << name << std::endl;
  for(auto name : faces)
    if(!showmissing || !file_exists_lsrc(name))
      std::cout << name << std::endl;
  for(auto name : tracks)
    if(!showmissing || !file_exists_lsrc(name))
      std::cout << name << std::endl;
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
