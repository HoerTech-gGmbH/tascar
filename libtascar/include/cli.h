/**
 * @file   cli.h
 * @author Giso Grimm
 *
 * @brief  Command line interface helper functions
 */
/*
 * License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef CLI_H
#define CLI_H

#include <getopt.h>
#include <map>
#include <string>

namespace TASCAR {

  void app_usage(const std::string& app_name, struct option* opt,
                 const std::string& app_arg = "", const std::string& help = "",
                 std::map<std::string, std::string> helpmap =
                     std::map<std::string, std::string>());

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
