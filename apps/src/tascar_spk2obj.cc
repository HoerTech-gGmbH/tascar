/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2024 Giso Grimm
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

#include "cli.h"
#include "speakerarray.h"
#include <fstream>
//#include "tscconfig.h"

int main(int argc, char** argv)
{
  std::string spkfile("");
  std::string objfile("");
  const char* options = "o:h";
  struct option long_options[] = {
      {"output", 1, 0, 'o'}, {"help", 0, 0, 'h'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'o':
      objfile = optarg;
      break;
    case 'h':
      TASCAR::app_usage("tascar_spk2obj", long_options, "<layout file>");
      return 0;
    }
  }
  if(optind < argc)
    spkfile = argv[optind++];
  if(spkfile.size() == 0) {
    TASCAR::app_usage("tascar_spk2obj", long_options, "<layout file>");
    return -1;
  }
  if(objfile.size() == 0) {
    objfile = spkfile + ".obj";
  }
  TASCAR::xml_doc_t doc(spkfile, TASCAR::xml_doc_t::LOAD_FILE);
  if(doc.root.get_element_name() != "layout")
    throw TASCAR::ErrMsg(
        "Invalid file type, expected root node type \"layout\", got \"" +
        doc.root.get_element_name() + "\".");
  TASCAR::spk_array_diff_render_t array(doc.root(), true);
  bool isflat = true;
  for(const auto& spk : array)
    if(spk.z != 0)
      isflat = false;
  std::ofstream objstream(objfile);
  objstream << "# TASCAR speaker layout coordinates" << std::endl;
  objstream << "# X: front, Y: left, Z: up" << std::endl;
  for(const auto& spk : array)
    objstream << "v " << spk.x << " " << spk.y << " " << spk.z << std::endl;
  if(isflat) {
    objstream << "l";
    for(size_t k = 0; k < array.size(); ++k)
      objstream << " " << k + 1;
    objstream << " 1" << std::endl;
  } else {
    std::vector<TASCAR::pos_t> spklist;
    for(const auto& spk : array)
      spklist.push_back(spk.normal());
    // calculate the convex hull:
    TASCAR::quickhull_t qh(spklist);
    for(const auto& face : qh.faces)
      objstream << "l " << face.c1 + 1 << " " << face.c2 + 1 << " "
                << face.c3 + 1 << " " << face.c1 + 1 << std::endl;
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
