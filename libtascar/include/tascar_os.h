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
#include <sys/types.h>

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

  /// @brief Match filename or pathname against pattern.
  /// On Linux and Mac, this function delegates to the system function fnmatch.
  /// @param pattern shell wildcard pattern against which to map string
  /// @param string string argument checked if it matches pattern
  /// @param fnm_pathname if true, then fnmatch matches a slash in string
  ///                     only with a slash in pattern and not by an
  ///                     asterisk (*) or a question mark (?) metacharacter,
  ///                     nor by a bracket expression ([]) containing a
  ///                     slash.  Other fnmatch modes are not supported.
  /// @bug On Windows, when fnm_pathname is true, this implementation only
  ///      checks that pattern and string contain the same number of
  ///      literal forward slashes, but does not check their placement.
  ///      We have not investigated if this results in the same behaviour
  ///      as on POSIX in all cases.
  /// @return 0 if string matches pattern
  /// @return nonzero value if there is no match or if there is an error
  int fnmatch(const char* pattern, const char* string, bool fnm_pathname);

  /// @brief return the canonicalized absolute pathname
  /// On Linux and Mac, this function delegates to the system function fnmatch.
  /// On Windows, this function TODO: describe.
  /// @param path input path
  /// @param resolved_path pointer to storage allocated by caller, at least
  ///                      MAX_PATH bytes long.  Must not be nullptr.
  /// @return this implementation returns resolved_path when the conversion
  ///         was successful, and returns path when not.
  const char* realpath(const char* path, char* resolved_path);

  /// @brief Spawn a subprocess and return its process ID
  /// This function is not implemented for Windows.
  /// @param command Command to be executed
  /// @param shell Launch command using a shell (true) or directly
  /// @param relaunch Relaunch the command when subprocess ended
  /// @return Process ID
  pid_t system(const char* command, bool shell);

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
