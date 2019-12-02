/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
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
#ifndef LICENSEHANDLER_H
#define LICENSEHANDLER_H

#include <libxml++/libxml++.h>
#include <string>
#include <map>
#include <set>

class licensehandler_t {
public:
  void add_license( const std::string& license, const std::string& attribution, const std::string& tag );
  void add_author( const std::string& author, const std::string& tag );
  bool has_authors() const;
  std::string get_authors() const;
  std::string legal_stuff(bool use_markup = false) const;
  std::string show_unknown() const;
  bool distributable() const;
private:
  std::map<std::string,std::set<std::string> > authors;
  std::map<std::string,std::set<std::string> > licenses;
  std::map<std::string,std::set<std::string> > attributions;
};

void get_license_info( xmlpp::Element* e, const std::string& fname, std::string& license, std::string& attribution );

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
