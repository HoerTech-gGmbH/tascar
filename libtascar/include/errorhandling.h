/**
 * @file   errorhandling.h
 * @author Giso Grimm
 * 
 * @brief  Basic error handling
 */ 
/* License (GPL)
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
#ifndef ERRORHANDLING_H
#define ERRORHANDLING_H

#include <string>

namespace TASCAR {

  class ErrMsg : public std::exception, private std::string {
  public:
    ErrMsg(const std::string& msg);
    virtual ~ErrMsg() throw();
    const char* what() const throw();
  };

}

#define TASCARProgErr(x) TASCAR::ErrMsg(std::string(__FILE__)+":"+std::to_string(__LINE__)+" Programming error: "+#x)

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
