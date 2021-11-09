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
#ifndef TASCAR_OS_H
#define TASCAR_OS_H
#include <time.h>

namespace TASCAR {
  /// @brief Convert a string representation of time to a time tm structure.
  ///
  /// On POSIX systems, this function delegates to the system function
  /// strptime.  On Windows, std::get_time is used instead.
  /// @param s string representatino of time
  /// @param f format string, for details see man 3 strptime
  /// @param tm pointer to structure in which to store the broken-down time
  /// @return pointer to first character that is not consumed by the conversion
  ///         or nullptr if s does not match format f.
  const char* strptime(const char* s, const char* f, struct tm* tm);

  /// @brief Function returning the platform-specific file name extension.
  /// @return ".dylib" on Mac
  /// @return ".dll" on Windows
  /// @return ".so" on Linux
  const char* dynamic_lib_extension(void);
} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
