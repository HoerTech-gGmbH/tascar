/* License (GPL)
 *
 * Copyright (C) 2021 Hörzentrum Oldenburg
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
#include "tascar_os.h"
#include <iomanip>
#include <sstream>

namespace TASCAR {
  const char* strptime(const char* s, const char* f, struct tm* tm)
  {
#ifndef _WIN32
    std::istringstream input(s);
    input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
    input >> std::get_time(tm, f);
    if(input.fail()) {
      return nullptr;
    }
    return s + input.tellg();
#else
    return ::strptime(s, f, tm);
#endif
  }
  const char* dynamic_lib_extension(void)
  {
#if defined(__APPLE__)
    return ".dylib";
#elif __linux__
    return +".so";
#elif _WIN32
    return ".dll";
#else
#error unsupported target platform
#endif
  }
} // namespace TASCAR

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
