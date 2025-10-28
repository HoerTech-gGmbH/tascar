/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cli.h"
#include "tscconfig.h"
#include <iostream>
#include <vector>
#if CMAKE
#include "tascarver.h"
#else
#include "../build/tascarver.h"
#endif

std::vector<std::string> linewrap(const std::string& s, size_t col)
{
  std::vector<std::string> out;
  std::string line;
  auto pars = TASCAR::str2vecstr(s, "\n");
  for(const auto& p : pars) {
    auto words = TASCAR::str2vecstr(p, " ");
    for(const auto& w : words) {
      if(line.size() + 1 + w.size() > col) {
        out.push_back(line);
        line = "";
      }
      if(line.size())
        line += " ";
      line += w;
    }
    out.push_back(line);
    line = "";
  }
  return out;
}

void TASCAR::app_usage(const std::string& app_name, struct option* opt,
                       const std::string& app_arg, const std::string& help,
                       std::map<std::string, std::string> helpmap)
{
  std::cout << "Usage:\n\n" << app_name << " [options] " << app_arg << "\n\n";
  if(!help.empty())
    std::cout << help << "\n\n";
  std::cout << "Options:\n\n";
  while(opt->name) {
    auto wrapped_help = linewrap(helpmap[opt->name], 55);
    std::string line1;
    line1 += "  -";
    line1 += (char)(opt->val);
    line1 += " ";
    if(opt->has_arg)
      line1 += "#";
    if(line1.size() < 20)
      line1 += std::string(20 - line1.size(), ' ');
    if(wrapped_help.size() > 0)
      line1 += wrapped_help[0];
    std::string line2;
    line2 += "  --" + std::string(opt->name);
    if(opt->has_arg)
      line2 += "=#";
    if(line2.size() < 20)
      line2 += std::string(20 - line2.size(), ' ');
    if(wrapped_help.size() > 1)
      line2 += wrapped_help[1];
    std::cout << line1 << "\n" << line2 << "\n";
    for(size_t k = 2; k < wrapped_help.size(); ++k)
      std::cout << std::string(20, ' ') << wrapped_help[k] << "\n";
    std::cout << "\n";
    opt++;
  }
  std::cout << std::endl << "(version: " << TASCARVER << ")" << std::endl;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
