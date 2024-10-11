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
#include "tscconfig.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "tascar.h"

TASCAR::spawn_process_t::spawn_process_t(const std::string& command,
                                         bool useshell, bool relaunch,
                                         double relaunchwait)
    : command_(command), useshell_(useshell), relaunch_(relaunch),
      relaunchwait_(relaunchwait)
{
  if(!command.empty()) {
    TASCAR::console_log("creating launcher for \"" + command + "\"" +
                        (useshell ? " shell" : "") +
                        (relaunch ? " relaunch" : ""));
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
    running = true;
    mtx.lock();
    pid = TASCAR::system(command_.c_str(), useshell_);
    mtx.unlock();
    console_log("started process " + TASCAR::to_string(pid) + " (" + command_ +
                ")");
#ifndef _WIN32
    int wstatus = 0;
    bool stillrunning = true;
    while(runservice && stillrunning) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      int opid = waitpid(pid, &wstatus, WNOHANG);
      if(opid != 0)
        stillrunning = false;
    }
    if(runservice) {
      if(WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) != 0)) {
        TASCAR::add_warning("Process " + TASCAR::to_string(pid) +
                            " returned with exit status " +
                            TASCAR::to_string(WEXITSTATUS(wstatus)) + ": \"" +
                            command_ + "\"" +
                            (relaunch_ ? (" Relaunching.") : ("")));
      }
      if(WIFSIGNALED(wstatus)) {
        auto termsig_ = WTERMSIG(wstatus);
        std::string termsigname;
        auto termsigname_c = strsignal(termsig_);
        if(termsigname_c)
          termsigname =
              std::string(" (") + std::string(termsigname_c) + std::string(")");
        TASCAR::add_warning(
            "Process " + TASCAR::to_string(pid) + " terminated with signal " +
            TASCAR::to_string(termsig_) + termsigname + ": \"" + command_ +
            "\"" + (relaunch_ ? (" Relaunching.") : ("")));
      }
    }
#else
    wait_for_process(pid);
#endif
    console_log("stopped process " + TASCAR::to_string(pid) + " (" + command_ +
                ")");
    pid = 0;
    running = false;
    if(relaunch_ && (relaunchwait_ > 0)) {
      tictoc_t tictoc;
      while(runservice && (tictoc.toc() < relaunchwait_))
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

TASCAR::spawn_process_t::~spawn_process_t()
{
  runservice = false;
  terminate_process(pid);
  if(launcherthread.joinable())
    launcherthread.join();
  if(command_.size())
    console_log("launcher for command \"" + command_ + "\" ended");
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
