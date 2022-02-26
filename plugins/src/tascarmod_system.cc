/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm, delude88
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

#include "session.h"
#ifndef _WIN32
#include "spawn_process.h"
#else
#include "tascar_os.h"
#include <thread>
#endif
#include <mutex>
#include <unistd.h>

class at_cmd_t {
public:
  at_cmd_t(tsccfg::node_t xmlsrc);
  at_cmd_t(uint32_t frame_, const std::string& cmd);
  void prepare(double f_sample)
  {
    if(!use_frame)
      frame = time * f_sample;
  };
  double time = 0.0;
  uint32_t frame = 0;
  std::string command;
  bool use_frame = false;
};

at_cmd_t::at_cmd_t(tsccfg::node_t xmlsrc) : time(0), frame(0), use_frame(false)
{
  TASCAR::xml_element_t e(xmlsrc);
  if(e.has_attribute("frame") && e.has_attribute("time"))
    TASCAR::add_warning("At-command has time and frame attribute, using frame.",
                        xmlsrc);
  if(e.has_attribute("frame"))
    use_frame = true;
  e.GET_ATTRIBUTE_(time);
  e.GET_ATTRIBUTE_(frame);
  e.GET_ATTRIBUTE_(command);
}

at_cmd_t::at_cmd_t(uint32_t frame_, const std::string& cmd)
{
  use_frame = true;
  frame = frame_;
  command = cmd;
}

class fifo_t {
public:
  fifo_t(uint32_t N);
  uint32_t read();
  void write(uint32_t v);
  bool can_write() const;
  bool can_read() const;

private:
  std::vector<uint32_t> data;
  uint32_t rpos;
  uint32_t wpos;
};

fifo_t::fifo_t(uint32_t N) : rpos(0), wpos(0)
{
  data.resize(N + 1);
}

uint32_t fifo_t::read()
{
  rpos = std::min((uint32_t)(data.size()) - 1u, rpos - 1u);
  return data[rpos];
}

void fifo_t::write(uint32_t v)
{
  wpos = std::min((uint32_t)(data.size()) - 1u, wpos - 1u);
  data[wpos] = v;
}

bool fifo_t::can_write() const
{
  return std::min((uint32_t)(data.size()) - 1u, wpos - 1u) != rpos;
}

bool fifo_t::can_read() const
{
  return wpos != rpos;
}

class system_t : public TASCAR::module_base_t {
public:
  system_t(const TASCAR::module_cfg_t& cfg);
  virtual ~system_t();
  virtual void release();
  virtual void update(uint32_t frame, bool running);
  virtual void configure();
  static int osc_trigger(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data);
  static int osc_atcmd_add(const char* path, const char* types, lo_arg** argv,
                           int argc, lo_message msg, void* user_data);
  static int osc_atcmd_clear(const char* path, const char* types, lo_arg** argv,
                             int argc, lo_message msg, void* user_data);
  void trigger(int32_t c);
  void trigger();
  void atcmdclear();
  void atcmdadd(double t, const std::string& cmd);

private:
  void service();
  std::string id;
  std::string command;
  std::string triggered;
  double sleep = 0.0;
  std::string onunload;
  bool noshell = true;
  bool relaunch = false;
  bool allowoscmod = false;
  bool timedcmdpipe = true;
  std::string timedprefix;
  FILE* h_atcmd = NULL;
  FILE* h_triggered = NULL;
#ifndef _WIN32
  TASCAR::spawn_process_t* proc = NULL;
  std::vector<TASCAR::spawn_process_t*> atprocs;
#endif
  fifo_t fifo;
  std::vector<at_cmd_t*> atcmds;
  std::thread srv;
  bool run_service;
  std::string sessionpath;
  std::mutex atcmdmtx;
};

int system_t::osc_trigger(const char* path, const char* types, lo_arg** argv,
                          int argc, lo_message msg, void* user_data)
{
  system_t* data(reinterpret_cast<system_t*>(user_data));
  if(data && (argc == 1) && (types[0] == 'i'))
    data->trigger(argv[0]->i);
  if(data && (argc == 0))
    data->trigger();
  return 0;
}

int system_t::osc_atcmd_add(const char* path, const char* types, lo_arg** argv,
                            int argc, lo_message msg, void* user_data)
{
  system_t* data(reinterpret_cast<system_t*>(user_data));
  if(data && (argc == 2) && (types[0] == 'f') && (types[1] == 's'))
    data->atcmdadd(argv[0]->f, &(argv[1]->s));
  return 0;
}

int system_t::osc_atcmd_clear(const char* path, const char* types,
                              lo_arg** argv, int argc, lo_message msg,
                              void* user_data)
{
  system_t* data(reinterpret_cast<system_t*>(user_data));
  if(data && (argc == 0))
    data->atcmdclear();
  return 0;
}

void system_t::trigger(int32_t c)
{
  if(!triggered.empty()) {
    char ctmp[1024];
    memset(ctmp, 0, sizeof(ctmp));
    snprintf(ctmp, sizeof(ctmp), "sh -c \"cd %s;%s %d\"", sessionpath.c_str(),
             triggered.c_str(), c);
    ctmp[sizeof(ctmp) - 1] = 0;
    if(h_triggered) {
      fprintf(h_triggered, "%s\n", ctmp);
      fflush(h_triggered);
    } else
      std::cerr << "Warning: no pipe\n";
  }
}

void system_t::trigger()
{
  if(!triggered.empty()) {
    char ctmp[1024];
    memset(ctmp, 0, sizeof(ctmp));
    snprintf(ctmp, sizeof(ctmp), "sh -c \"cd %s;%s\"", sessionpath.c_str(),
             triggered.c_str());
    ctmp[sizeof(ctmp) - 1] = 0;
    if(h_triggered) {
      fprintf(h_triggered, "%s\n", ctmp);
      fflush(h_triggered);
    } else
      std::cerr << "Warning: no pipe\n";
  }
}

system_t::system_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), id("system"), fifo(1024), run_service(true),
      sessionpath(session->get_session_path())
{
  GET_ATTRIBUTE_(id);
  GET_ATTRIBUTE(command, "", "command to be executed");
  GET_ATTRIBUTE(
      sleep, "s",
      "wait after starting the command before continuing to load session");
  GET_ATTRIBUTE(onunload, "", "command to be executed when unloading session");
  GET_ATTRIBUTE(triggered, "", "command to be executed upon trigger signal");
  GET_ATTRIBUTE_BOOL(noshell, "do not use shell to spawn subprocess");
  GET_ATTRIBUTE_BOOL(relaunch,
                     "relaunch process if ended before session unload");
  GET_ATTRIBUTE_BOOL(allowoscmod,
                     "allow modifications of timed commands via OSC");
  GET_ATTRIBUTE(timedprefix, "", "Prefix for timed commands added via OSC");
  GET_ATTRIBUTE_BOOL(
      timedcmdpipe, "start timed commands using a pipe (true) or fork (false)");
  atcmdmtx.lock();
  for(auto sne : tsccfg::node_get_children(e, "at"))
    atcmds.push_back(new at_cmd_t(sne));
  atcmdmtx.unlock();
  if(!command.empty()) {
#ifdef _WIN32
    throw TASCAR::ErrMsg(
        "Spawning sub-processes on Windows is not yet implemented: " + command);
#else
    proc = new TASCAR::spawn_process_t(TASCAR::env_expand(command), !noshell,
                                       relaunch);
#endif
  }
  if(atcmds.size() || allowoscmod) {
    h_atcmd = popen("/bin/bash -s", "w");
    if(!h_atcmd)
      throw TASCAR::ErrMsg("Unable to create pipe with /bin/bash");
    fprintf(h_atcmd, "cd %s\n", sessionpath.c_str());
    fflush(h_atcmd);
  }
  if(triggered.size()) {
    h_triggered = popen("/bin/bash -s", "w");
    if(!h_triggered)
      throw TASCAR::ErrMsg(
          "Unable to create pipe for triggered command with /bin/bash");
    fprintf(h_triggered, "cd %s\n", sessionpath.c_str());
    fflush(h_triggered);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000.0 * sleep)));

  if(!triggered.empty()) {
    session->add_method("/" + id + "/trigger", "i", &system_t::osc_trigger,
                        this);
    session->add_method("/" + id + "/trigger", "", &system_t::osc_trigger,
                        this);
  }
  if(allowoscmod) {
    session->add_method("/" + id + "/timed/add", "fs", &system_t::osc_atcmd_add,
                        this);
    session->add_method("/" + id + "/timed/clear", "",
                        &system_t::osc_atcmd_clear, this);
  }
  run_service = true;
  srv = std::thread(&system_t::service, this);
}

void system_t::update(uint32_t frame, bool running)
{
  if(running)
    if(atcmdmtx.try_lock()) {
      for(uint32_t k = 0; k < atcmds.size(); k++)
        if((frame <= atcmds[k]->frame) &&
           (atcmds[k]->frame < frame + n_fragment))
          if(fifo.can_write())
            fifo.write(k);
      atcmdmtx.unlock();
    }
}

void system_t::configure()
{
  module_base_t::configure();
  atcmdmtx.lock();
  for(auto cmd : atcmds)
    cmd->prepare(f_sample);
  atcmdmtx.unlock();
}

void system_t::release()
{
  module_base_t::release();
}

void system_t::atcmdclear()
{
  atcmdmtx.lock();
  for(auto cmd : atcmds)
    delete cmd;
  atcmds.clear();
#ifndef _WIN32
  for(auto proc : atprocs)
    delete proc;
  atprocs.clear();
#endif
  atcmdmtx.unlock();
}

void system_t::atcmdadd(double t, const std::string& cmd)
{
  atcmdmtx.lock();
  atcmds.push_back(new at_cmd_t(t * f_sample, timedprefix + cmd));
  atcmdmtx.unlock();
}

system_t::~system_t()
{
  run_service = false;
  srv.join();
#ifndef _WIN32
  if(proc)
    delete proc;
#endif
  atcmdclear();
  if(h_atcmd)
    fclose(h_atcmd);
  if(h_triggered)
    fclose(h_triggered);
  if(!onunload.empty()) {
    int err(system(onunload.c_str()));
    if(err != 0)
      std::cerr << "subprocess returned " << err << std::endl;
  }
}

void system_t::service()
{
#ifndef _WIN32
  while(run_service) {
    usleep(500);
    if(fifo.can_read()) {
      uint32_t v(fifo.read());
      std::string cmd;
      atcmdmtx.lock();
      if(v < atcmds.size())
        cmd = atcmds[v]->command.c_str();
      atcmdmtx.unlock();
      if(cmd.size()) {
        if(timedcmdpipe) {
          if(h_atcmd) {
            fprintf(h_atcmd, "%s\n", cmd.c_str());
            fflush(h_atcmd);
          } else
            std::cerr << "Warning: no pipe\n";
        } else {
          atprocs.push_back(new TASCAR::spawn_process_t(cmd, false, false));
        }
      }
    }
  }
#endif
}

REGISTER_MODULE(system_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
