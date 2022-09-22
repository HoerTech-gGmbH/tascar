/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2022 Giso Grimm
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

#include "spawn_process.h"
#include "tascar_os.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "tascar.h"

TASCAR::spawn_process_t::spawn_process_t(const std::string& command,
                                         bool useshell, bool relaunch)
    : command_(command), useshell_(useshell), relaunch_(relaunch)
{
  if(!command.empty()) {
    runservice = true;
    launcherthread = std::thread(&spawn_process_t::launcher, this);
  }
  mtx.lock();
  mtx.unlock();
}

void TASCAR::spawn_process_t::launcher()
{
  bool first = true;
  while(runservice && (first || relaunch_)) {
    first = false;
    int wstatus = 0;
    running = true;
    mtx.lock();
    pid = TASCAR::system(command_.c_str(), useshell_);
    mtx.unlock();
    waitpid(pid, &wstatus, 0);
    if(runservice) {
      if(WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) != 0))
        std::cerr << "Process " << pid << " returned with exit status "
                  << WEXITSTATUS(wstatus) << ": \"" << command_ << "\""
                  << std::endl;
      if(WIFSIGNALED(wstatus))
        std::cerr << "Process " << pid << " terminated with signal "
                  << WTERMSIG(wstatus) << ": \"" << command_ << "\""
                  << std::endl;
    }
    pid = 0;
    running = false;
  }
}

TASCAR::spawn_process_t::~spawn_process_t()
{
  runservice = false;
  if(pid != 0) {
    killpg(pid, SIGTERM);
  }
  if(launcherthread.joinable())
    launcherthread.join();
}

void TASCAR::spawn_process_t::set_relaunch(bool relaunch)
{
  relaunch_ = relaunch;
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
