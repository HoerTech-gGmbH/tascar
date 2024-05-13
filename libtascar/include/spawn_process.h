/**
 * @file spawn_process.h
 * @brief Spawn a subprocess
 * @ingroup libtascar
 * @author Giso Grimm
 * @date 2022
 */
/* License (GPL)
 *
 * Copyright (C) 2022 Giso Grimm
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

#ifndef SPAWN_PROCESS_H
#define SPAWN_PROCESS_H

#include <mutex>
#include <string>
#include <thread>

namespace TASCAR {
  /**
   * \brief Spawn a process in the background and stop it
   */
  class spawn_process_t {
  public:
    spawn_process_t(const std::string& command, bool useshell = true,
                    bool relaunch = false, double relaunchwait = 0.0);
    ~spawn_process_t();
    void set_relaunch(bool relaunch);

  private:
    void launcher();
    std::thread launcherthread;
    pid_t pid = 0;
    bool runservice = true;
    std::string command_;
    bool useshell_ = false;
    bool relaunch_ = false;
    double relaunchwait_ = 0.0;
    bool running = false;
    std::mutex mtx;
  };
} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
