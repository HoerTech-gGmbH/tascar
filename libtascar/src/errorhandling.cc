/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "defs.h"
#include "errorhandling.h"

TASCAR::ErrMsg::ErrMsg(const std::string& msg) 
  : std::string(msg) 
{
#ifdef TSCDEBUG
  std::cerr << "-Error: " << msg << std::endl;
#endif
}

TASCAR::ErrMsg::~ErrMsg() throw()
{
}

const char* TASCAR::ErrMsg::what() const throw ()
{
  return c_str(); 
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
