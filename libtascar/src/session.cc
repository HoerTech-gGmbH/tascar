/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm, Tobias Hegemann
 * Copyright (c) 2021 Giso Grimm
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
#include "tascar_os.h"
#include <chrono>
#include <dlfcn.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <thread>
#include <unistd.h>

/**
   \defgroup moddev Module development

   The functionality of TASCAR can be extended using modules. Five
   types of modules can be used in TASCAR:

   \li general purpose modules (derived from TASCAR::module_base_t)
   \li actor modules (derived from TASCAR::actor_module_t)
   \li audio processing plugins (derived from TASCAR::audioplugin_base_t)
   \li source modules (derived from TASCAR::sourcemod_base_t)
   \li receiver modules (derived from TASCAR::receivermod_base_t)

   All of these module types consist of an initialization block which
   is called when the module is loaded, a prepare() and release()
   method pair, which is called when the audio properties such as
   sampling rate and block size are defined, and an update method,
   which is called once in each processing cycle.

   General purpose modules and actor modules are updated in
   TASCAR::session_t::process(), which is called first, before the
   geometry update and acoustic modelling of the scenes.

   Receiver modules, source modules and audio processing plugins are
   updated at their appropriate place during acoustic modelling, i.e,
   after the geometry update in TASCAR::render_core_t::process().

   To develop your own modules, copy the folder
   /usr/share/tascar/examples/plugins to a destination of your choice,
   e.g.:

   \verbatim
   cp -r /usr/share/tascar/examples/plugins ./
   \endverbatim

   Change to the new plugins directory:

   \verbatim
   cd plugins
   \endverbatim

   Now modify the Makefile and the appropriate source files in the "src"
   folder to your needs. For compilation, type:

   \verbatim
   make
   \endverbatim

   The plugins can be tested by starting tascar with:
   \verbatim
   LD_LIBRARY_PATH=./build tascar sessionfile.tsc
   \endverbatim

   Replace "sessionfile.tsc" by the name of your session file, e.g.,
   "example_rotate.tsc".

   To install the plugins, type:
   \verbatim
   sudo make install
   \endverbatim
   After a TASCAR update, a recompilation is required:
   \verbatim
   make clean
   make
   sudo make install
   \endverbatim

   \note All external modules should use SI units for signals and variables.


   \section actormod Actor modules

   An example of an actor module is provided in \ref tascar_rotate.cc .

   Actor modules can interact with the position of objects. The
   objects can be selected with the \c actor attribute (see the <a
   href="file:///usr/share/doc/tascar/manual.pdf#section.6">user
   manual, section 6</a> for details).

   The core functionality is implemented in the update() method. Here
   you may access the list of actor objects,
   TASCAR::actor_module_t::obj, or use any of the following functions
   to control the delta-transformation of the objects:

   <ul>
   <li>TASCAR::actor_module_t::set_location()</li>
   <li>TASCAR::actor_module_t::set_orientation()</li>
   <li>TASCAR::actor_module_t::set_transformation()</li>
   <li>TASCAR::actor_module_t::add_location()</li>
   <li>TASCAR::actor_module_t::add_orientation()</li>
   <li>TASCAR::actor_module_t::add_transformation()</li>
   </ul>

   Variables can be registered for XML access (static) or OSC access
   (interactive) in the constructor. To register a variable for XML
   access, use the macro GET_ATTRIBUTE(). OSC variables can be
   registered with TASCAR::osc_server_t::add_double() and similar
   member functions of TASCAR::osc_server_t.

   The parameters of the audio signal backend, e.g., sampling rate and
   fragment size, can be accessed via the public attributes of
   chunk_cfg_t.

   \section audioplug Audio plugins

   An example of an audio plugin is provided in \ref
   tascar_ap_noise.cc .

   Audio plugins can be used in sounds or in receivers (see the <a
   href="file:///usr/share/doc/tascar/manual.pdf#section.7">user
   manual, section 7</a> for more details). In sounds,
   they are processed before application of the acoustic model, and in
   receivers, they are processed after rendering, in the receiver
   post_proc method.

   \section sourcemod Source modules

   An example of a source module is provided in \ref
   tascarsource_example.cc .

   Source directivity types are properties of sounds (see the <a
   href="file:///usr/share/doc/tascar/manual.pdf#subsection.5.3">user
   manual, subsection 5.3</a> for more details).

   \section receivermod Receiver modules

   An example of a receiver module is provided in \ref
   tascarreceiver_example.cc .

   For use documentation of receivers, see the <a
   href="file:///usr/share/doc/tascar/manual.pdf#subsection.5.5">user
   manual, subsection 5.5</a>.

   \example tascar_rotate.cc

   \example tascar_ap_noise.cc

   \example tascarsource_example.cc

   \example tascarreceiver_example.cc

*/

using namespace TASCAR;

TASCAR_RESOLVER(module_base_t, const module_cfg_t&)

TASCAR::module_t::module_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), lib(NULL), libdata(NULL)
{
  name = tsccfg::node_get_name(e);
  std::string libname("tascar_");
#ifdef PLUGINPREFIX
  libname = PLUGINPREFIX + libname;
#endif
  libname += name + TASCAR::dynamic_lib_extension();
  lib = dlopen((TASCAR::get_libdir() + libname).c_str(), RTLD_NOW);
  if(!lib)
    throw TASCAR::ErrMsg("Unable to open module \"" + name +
                         "\": " + dlerror());
  try {
    module_base_t_resolver(&libdata, cfg, lib, libname);
  }
  catch(...) {
    dlclose(lib);
    throw;
  }
}

void TASCAR::module_t::update(uint32_t frame, bool running)
{
  if(is_prepared())
    libdata->update(frame, running);
}

void TASCAR::module_t::configure()
{
  module_base_t::configure();
  try {
    libdata->prepare(cfg());
  }
  catch(...) {
    module_base_t::release();
    throw;
  }
}

void TASCAR::module_t::post_prepare()
{
  module_base_t::post_prepare();
  libdata->post_prepare();
}

void TASCAR::module_t::release()
{
  module_base_t::release();
  libdata->release();
}

void TASCAR::module_t::validate_attributes(std::string& msg) const
{
  libdata->validate_attributes(msg);
}

TASCAR::module_t::~module_t()
{
  delete libdata;
  dlclose(lib);
}

TASCAR::module_cfg_t::module_cfg_t(tsccfg::node_t xmlsrc_,
                                   TASCAR::session_t* session_)
    : session(session_), xmlsrc(xmlsrc_)
{
}

tsccfg::node_t assert_element(tsccfg::node_t e)
{
  if(!e)
    throw TASCAR::ErrMsg("NULL pointer element");
  return e;
}

const std::string& debug_str(const std::string& s)
{
  return s;
}

TASCAR::session_oscvars_t::session_oscvars_t(tsccfg::node_t src)
    : xml_element_t(src), name("tascar"), srv_port("9877"), srv_proto("UDP")
{
  GET_ATTRIBUTE(srv_port, "", "OSC port number");
  GET_ATTRIBUTE(srv_addr, "", "OSC multicast address in case of UDP transport");
  GET_ATTRIBUTE(srv_proto, "", "OSC protocol, UDP or TCP");
  GET_ATTRIBUTE(name, "", "session name");
  GET_ATTRIBUTE(starturl, "", "URL of start page for display");
}

TASCAR::session_core_t::session_core_t()
    : duration(60), loop(false), playonload(false), levelmeter_tc(2.0),
      levelmeter_weight(TASCAR::levelmeter::Z), levelmeter_min(30.0),
      levelmeter_range(70.0), requiresrate(0), warnsrate(0), requirefragsize(0),
      warnfragsize(0), initcmdsleep(0), h_pipe_initcmd(NULL), pid_initcmd(0)
{
  root.GET_ATTRIBUTE(duration, "s", "session duration");
  root.GET_ATTRIBUTE_BOOL(loop, "loop session at end");
  root.GET_ATTRIBUTE_BOOL(playonload, "start playing when session is loaded");
  root.GET_ATTRIBUTE(levelmeter_tc, "s", "level meter time constant");
  root.GET_ATTRIBUTE_NOUNIT(levelmeter_weight, "level meter weighting");
  root.GET_ATTRIBUTE(levelmeter_mode, "",
                     "Level meter mode (rms, rmspeak, percentile)");
  root.GET_ATTRIBUTE(levelmeter_min, "dB SPL", "Level meter minimum");
  root.GET_ATTRIBUTE(levelmeter_range, "dB", "Level range of level meters");
  root.GET_ATTRIBUTE(
      requiresrate, "Hz",
      "Session sampling rate, stop loading the session if the system "
      "sampling rate doesn't match");
  root.GET_ATTRIBUTE(
      requirefragsize, "",
      "Session fragment size, stop loading the session if the system "
      "fragment size doesn't match");
  root.GET_ATTRIBUTE(
      warnsrate, "Hz",
      "Session sampling rate, print a warning if the system sampling "
      "rate doesn't match");
  root.GET_ATTRIBUTE(
      warnfragsize, "",
      "Session fragment size, print a warning if the system fragment "
      "size doesn't match");
  root.GET_ATTRIBUTE(
      initcmd, "",
      "Command to be executed before first connection to jack. Can "
      "be used to start jack server.");
  root.GET_ATTRIBUTE(initcmdsleep, "s",
                     "Time to wait for initcmd to start up, in seconds.");
  start_initcmd();
}

TASCAR::session_core_t::session_core_t(const std::string& filename_or_data,
                                       load_type_t t, const std::string& path)
    : TASCAR::tsc_reader_t(filename_or_data, t, path), duration(60),
      loop(false), playonload(false), levelmeter_tc(2.0),
      levelmeter_weight(TASCAR::levelmeter::Z), levelmeter_min(30.0),
      levelmeter_range(70.0), requiresrate(0), warnsrate(0), requirefragsize(0),
      warnfragsize(0), initcmdsleep(0), h_pipe_initcmd(NULL), pid_initcmd(0)
{
  root.GET_ATTRIBUTE(duration, "s", "session duration");
  root.GET_ATTRIBUTE_BOOL(loop, "loop session at end");
  root.GET_ATTRIBUTE_BOOL(playonload, "start playing when session is loaded");
  root.GET_ATTRIBUTE(levelmeter_tc, "s", "level meter time constant");
  root.GET_ATTRIBUTE_NOUNIT(levelmeter_weight, "level meter weighting");
  root.GET_ATTRIBUTE(levelmeter_mode, "",
                     "Level meter mode (rms, rmspeak, percentile)");
  root.GET_ATTRIBUTE(levelmeter_min, "dB SPL", "Level meter minimum");
  root.GET_ATTRIBUTE(levelmeter_range, "dB", "Level range of level meters");
  root.GET_ATTRIBUTE(
      requiresrate, "Hz",
      "Session sampling rate, stop loading the session if the system "
      "sampling rate doesn't match");
  root.GET_ATTRIBUTE(
      requirefragsize, "",
      "Session fragment size, stop loading the session if the system "
      "fragment size doesn't match");
  root.GET_ATTRIBUTE(
      warnsrate, "Hz",
      "Session sampling rate, print a warning if the system sampling "
      "rate doesn't match");
  root.GET_ATTRIBUTE(
      warnfragsize, "",
      "Session fragment size, print a warning if the system fragment "
      "size doesn't match");
  root.GET_ATTRIBUTE(
      initcmd, "",
      "Command to be executed before first connection to jack. Can "
      "be used to start jack server.");
  root.GET_ATTRIBUTE(initcmdsleep, "s",
                     "Time to wait for initcmd to start up, in seconds.");
  start_initcmd();
}

void TASCAR::session_core_t::start_initcmd()
{
  if(!initcmd.empty()) {
#ifdef _WIN32
    add_warning("Not executing initcmd on Windows: " + initcmd);
#else
    char ctmp[1024];
    memset(ctmp, 0, sizeof(ctmp));
    snprintf(ctmp, sizeof(ctmp), "sh -c \"%s >/dev/null & echo \\$!;\"",
             initcmd.c_str());
    ctmp[sizeof(ctmp) - 1] = 0;
    h_pipe_initcmd = popen(ctmp, "r");
    if(fgets(ctmp, 1024, h_pipe_initcmd) != NULL) {
      pid_initcmd = atoi(ctmp);
      if(pid_initcmd == 0) {
        std::cerr << "Warning: Invalid subprocess PID (while attempting to "
                     "start init command \""
                  << initcmd << "\")." << std::endl;
      }
    }
#endif
    if(initcmdsleep > 0)
      std::this_thread::sleep_for(
          std::chrono::milliseconds((int)(1000.0 * initcmdsleep)));
  }
}

TASCAR::session_core_t::~session_core_t()
{
#ifndef _WIN32
  if(pid_initcmd != 0)
    kill(pid_initcmd, SIGTERM);
  if(h_pipe_initcmd)
    fclose(h_pipe_initcmd);
#endif
}

void assert_jackpar(const std::string& what, double expected, double found,
                    bool warn, const std::string& unit = "")
{
  if((expected > 0) && (expected != found)) {
    std::string msg("Invalid " + what + " (expected " +
                    TASCAR::to_string(expected) + unit + ", jack has " +
                    TASCAR::to_string(found) + unit + ")");
    if(warn)
      TASCAR::add_warning(msg);
    else
      throw TASCAR::ErrMsg(msg);
  }
}

TASCAR::session_t::session_t()
    : TASCAR::session_oscvars_t(root()),
      jackc_transport_t(jacknamer(name, "session.")),
      osc_server_t(srv_addr, srv_port, srv_proto,
                   TASCAR::config("tascar.osc.list", 0)),
      period_time(1.0 / (double)srate), started_(false)
{
  assert_jackpar("sampling rate", requiresrate, srate, false, " Hz");
  assert_jackpar("fragment size", requirefragsize, fragsize, false);
  assert_jackpar("sampling rate", warnsrate, srate, true, " Hz");
  assert_jackpar("fragment size", warnfragsize, fragsize, true);
  profilermsg = lo_message_new();
  pthread_mutex_init(&mtx, NULL);
  read_xml();
  try {
    add_output_port("sync_out");
    jackc_transport_t::activate();
    add_transport_methods();
    osc_server_t::activate();
    if(playonload)
      tp_start();
  }
  catch(...) {
    jackc_transport_t::deactivate();
    unload_modules();
    lo_message_free(profilermsg);
    throw;
  }
  profilermsgargv = lo_message_get_argv(profilermsg);
  if( use_profiler ){
    std::cout << "<osc path=\"" << profilingpath << "\" size=\"" << modules.size() << "\"/>" << std::endl;
    std::cout << "csModules = { ";
    for(auto mod : modules)
      std::cout << "'" << mod->modulename() << "' ";
    std::cout << "};" << std::endl;
  }
}

TASCAR::session_t::session_t(const std::string& filename_or_data, load_type_t t,
                             const std::string& path)
    : TASCAR::session_core_t(filename_or_data, t, path),
      session_oscvars_t(root()), jackc_transport_t(jacknamer(name, "session.")),
      osc_server_t(srv_addr, srv_port, srv_proto,
                   TASCAR::config("tascar.osc.list", 0)),
      period_time(1.0 / (double)srate), started_(false)
{
  assert_jackpar("sampling rate", requiresrate, srate, false, " Hz");
  assert_jackpar("fragment size", requirefragsize, fragsize, false);
  assert_jackpar("sampling rate", warnsrate, srate, true, " Hz");
  assert_jackpar("fragment size", warnfragsize, fragsize, true);
  profilermsg = lo_message_new();
  pthread_mutex_init(&mtx, NULL);
  // parse XML:
  read_xml();
  try {
    add_output_port("sync_out");
    jackc_transport_t::activate();
    add_transport_methods();
    osc_server_t::activate();
    if(playonload)
      tp_start();
  }
  catch(...) {
    jackc_transport_t::deactivate();
    unload_modules();
    lo_message_free(profilermsg);
    throw;
  }
  profilermsgargv = lo_message_get_argv(profilermsg);
  if( use_profiler ){
    std::cout << "<osc path=\"" << profilingpath << "\" size=\"" << modules.size() << "\"/>" << std::endl;
    std::cout << "csModules = { ";
    for(auto mod : modules)
      std::cout << "'" << mod->modulename() << "' ";
    std::cout << "};" << std::endl;
  }
}

void TASCAR::session_t::read_xml()
{
  try {
    TASCAR::tsc_reader_t::read_xml();
  }
  catch(...) {
    if(lock_vars()) {
      std::vector<TASCAR::module_t*> lmodules(modules);
      modules.clear();
      for(std::vector<TASCAR::module_t*>::iterator it = lmodules.begin();
          it != lmodules.end(); ++it)
        if((*it)->is_prepared())
          (*it)->release();
      for(std::vector<TASCAR::module_t*>::iterator it = lmodules.begin();
          it != lmodules.end(); ++it)
        delete(*it);
      for(std::vector<TASCAR::scene_render_rt_t*>::iterator it = scenes.begin();
          it != scenes.end(); ++it)
        delete(*it);
      for(std::vector<TASCAR::range_t*>::iterator it = ranges.begin();
          it != ranges.end(); ++it)
        delete(*it);
      for(std::vector<TASCAR::connection_t*>::iterator it = connections.begin();
          it != connections.end(); ++it)
        delete(*it);
      unlock_vars();
    }
    throw;
  }
}

void TASCAR::session_t::unload_modules()
{
  if(started_)
    stop();
  if(lock_vars()) {
    std::vector<TASCAR::module_t*> lmodules(modules);
    modules.clear();
    for(auto it = lmodules.begin(); it != lmodules.end(); ++it) {
      if((*it)->is_prepared())
        (*it)->release();
    }
    for(auto it = lmodules.begin(); it != lmodules.end(); ++it) {
      delete(*it);
    }
    for(auto it = scenes.begin(); it != scenes.end(); ++it)
      delete(*it);
    scenes.clear();
    for(auto it = ranges.begin(); it != ranges.end(); ++it)
      delete(*it);
    ranges.clear();
    for(auto it = connections.begin(); it != connections.end(); ++it)
      delete(*it);
    connections.clear();
    unlock_vars();
  }
}

bool TASCAR::session_t::lock_vars()
{
  return (pthread_mutex_lock(&mtx) == 0);
}

void TASCAR::session_t::unlock_vars()
{
  pthread_mutex_unlock(&mtx);
}

bool TASCAR::session_t::trylock_vars()
{
  return (pthread_mutex_trylock(&mtx) == 0);
}

TASCAR::session_t::~session_t()
{
  osc_server_t::deactivate();
  jackc_transport_t::deactivate();
  unload_modules();
  pthread_mutex_trylock(&mtx);
  pthread_mutex_unlock(&mtx);
  pthread_mutex_destroy(&mtx);
  lo_message_free(profilermsg);
}

std::vector<std::string> TASCAR::session_t::get_render_output_ports() const
{
  std::vector<std::string> ports;
  for(std::vector<TASCAR::scene_render_rt_t*>::const_iterator it =
          scenes.begin();
      it != scenes.end(); ++it) {
    std::vector<std::string> pports((*it)->get_output_ports());
    ports.insert(ports.end(), pports.begin(), pports.end());
  }
  return ports;
}

void TASCAR::session_t::add_scene(tsccfg::node_t src)
{
  TASCAR::scene_render_rt_t* newscene(NULL);
  if(!src)
    src = root.add_child("scene");
  try {
    newscene = new TASCAR::scene_render_rt_t(src);
    if(namelist.find(newscene->name) != namelist.end())
      throw TASCAR::ErrMsg("A scene of name \"" + newscene->name +
                           "\" already exists in the session.");
    namelist.insert(newscene->name);
    scenes.push_back(newscene);
    scenes.back()->configure_meter((float)levelmeter_tc, levelmeter_weight);
    scenemap[newscene->id] = newscene;
    for(auto& sound : newscene->sounds) {
      const std::string id(sound->get_id());
      auto mapsnd(soundmap.find(id));
      if(mapsnd != soundmap.end()) {
        throw TASCAR::ErrMsg("The sound id \"" + id +
                             "\" is not unique (already used by source \"" +
                             soundmap[id]->get_parent_name() +
                             "\" in scene \"" + newscene->name + "\").");
      } else {
        soundmap[sound->get_id()] = sound;
      }
    }
    for(auto& source : newscene->source_objects) {
      const std::string id(source->get_id());
      auto mapsrc(sourcemap.find(id));
      if(mapsrc != sourcemap.end()) {
        throw TASCAR::ErrMsg("The sound id \"" + id +
                             "\" is not unique (already used by source \"" +
                             sourcemap[id]->get_name() + "\" in scene \"" +
                             newscene->name + "\").");
      } else {
        sourcemap[source->get_id()] = source;
      }
    }
    for(auto& rec : newscene->receivermod_objects) {
      const std::string id(rec->get_id());
      auto maprec(receivermap.find(id));
      if(maprec != receivermap.end()) {
        throw TASCAR::ErrMsg("The sound id \"" + id +
                             "\" is not unique (already used by source \"" +
                             receivermap[id]->get_name() + "\" in scene \"" +
                             newscene->name + "\").");
      } else {
        receivermap[rec->get_id()] = rec;
      }
    }
  }
  catch(...) {
    delete newscene;
    throw;
  }
}

TASCAR::Scene::sound_t& TASCAR::session_t::sound_by_id(const std::string& id)
{
  auto snd(soundmap.find(id));
  if(snd != soundmap.end())
    return *(snd->second);
  throw TASCAR::ErrMsg("Unknown sound id \"" + id + "\" in session.");
}

TASCAR::scene_render_rt_t& TASCAR::session_t::scene_by_id(const std::string& id)
{
  auto scene(scenemap.find(id));
  if(scene != scenemap.end())
    return *(scene->second);
  throw TASCAR::ErrMsg("Unknown scene id \"" + id + "\" in session \"" + name +
                       "\".");
}

TASCAR::Scene::src_object_t&
TASCAR::session_t::source_by_id(const std::string& id)
{
  auto src(sourcemap.find(id));
  if(src != sourcemap.end())
    return *(src->second);
  throw TASCAR::ErrMsg("Unknown source id \"" + id + "\" in session.");
}

TASCAR::Scene::receiver_obj_t&
TASCAR::session_t::receiver_by_id(const std::string& id)
{
  auto rec(receivermap.find(id));
  if(rec != receivermap.end())
    return *(rec->second);
  throw TASCAR::ErrMsg("Unknown receiver id \"" + id + "\" in session \"" +
                       name + "\".");
}

void TASCAR::session_t::add_range(tsccfg::node_t src)
{
  if(!src)
    src = root.add_child("range");
  ranges.push_back(new TASCAR::range_t(src));
}

void TASCAR::session_t::add_connection(tsccfg::node_t src)
{
  if(!src)
    src = root.add_child("connect");
  connections.push_back(new TASCAR::connection_t(src));
}

void TASCAR::session_t::add_module(tsccfg::node_t src)
{
  if(!src)
    src = root.add_child("module");
  modules.push_back(new TASCAR::module_t(TASCAR::module_cfg_t(src, this)));
  lo_message_add_double(profilermsg, 0.0);
}

void TASCAR::session_t::start()
{
  started_ = true;
  try {
    for(std::vector<TASCAR::scene_render_rt_t*>::iterator ipl = scenes.begin();
        ipl != scenes.end(); ++ipl) {
      (*ipl)->start();
      (*ipl)->add_child_methods(this);
    }
  }
  catch(...) {
    started_ = false;
    throw;
  }
  auto last_prepared(modules.begin());
  try {
    for(std::vector<TASCAR::module_t*>::iterator imod = modules.begin();
        imod != modules.end(); ++imod) {
      chunk_cfg_t cf(srate, fragsize);
      last_prepared = imod;
      (*imod)->prepare(cf);
    }
    last_prepared = modules.end();
  }
  catch(...) {
    for(auto it = modules.begin(); it != last_prepared; ++it)
      (*it)->release();
    started_ = false;
    throw;
  }
  for(auto& mod : modules)
    mod->post_prepare();
  for(std::vector<TASCAR::connection_t*>::iterator icon = connections.begin();
      icon != connections.end(); ++icon) {
    connect((*icon)->src, (*icon)->dest, !(*icon)->failonerror, true, true);
  }
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator ipl = scenes.begin();
      ipl != scenes.end(); ++ipl) {
    try {
      connect(get_client_name() + ":sync_out",
              (*ipl)->get_client_name() + ":sync_in");
    }
    catch(const std::exception& e) {
      add_warning(e.what());
    }
    (*ipl)->add_licenses(this);
  }
}

int TASCAR::session_t::process(jack_nframes_t, const std::vector<float*>&,
                               const std::vector<float*>&, uint32_t tp_frame,
                               bool tp_rolling)
{
  double t(period_time * (double)tp_frame);
  uint32_t next_tp_frame(tp_frame);
  if(tp_rolling)
    next_tp_frame += fragsize;
  if(started_) {
    double t_prev = 0.0;
    size_t k = 0;
    if(use_profiler)
      tictoc.tic();
    for(auto mod : modules) {
      mod->update(next_tp_frame, tp_rolling);
      if(use_profiler) {
        auto tloc = tictoc.toc();
        profilermsgargv[k]->d = tloc - t_prev;
        t_prev = tloc;
      }
      ++k;
    }
    if(use_profiler)
      dispatch_data_message(profilingpath.c_str(), profilermsg);
  }
  if((duration > 0) && (t >= duration)) {
    if(loop)
      tp_locate(0u);
    else
      tp_stop();
  }
  return 0;
}

void TASCAR::session_t::stop()
{
  started_ = false;
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator ipl = scenes.begin();
      ipl != scenes.end(); ++ipl)
    (*ipl)->stop();
}

void TASCAR::session_t::run(bool& b_quit, bool use_stdin)
{
  start();
  while(!b_quit) {
    usleep(50000);
    if(use_stdin) {
      getchar();
      if(feof(stdin))
        b_quit = true;
    }
  }
  stop();
}

uint32_t TASCAR::session_t::get_active_pointsources() const
{
  uint32_t rv(0);
  for(std::vector<TASCAR::scene_render_rt_t*>::const_iterator it =
          scenes.begin();
      it != scenes.end(); ++it)
    rv += (*it)->active_pointsources;
  return rv;
}

uint32_t TASCAR::session_t::get_total_pointsources() const
{
  uint32_t rv(0);
  for(std::vector<TASCAR::scene_render_rt_t*>::const_iterator it =
          scenes.begin();
      it != scenes.end(); ++it)
    rv += (*it)->total_pointsources;
  return rv;
}

uint32_t TASCAR::session_t::get_active_diffuse_sound_fields() const
{
  uint32_t rv(0);
  for(std::vector<TASCAR::scene_render_rt_t*>::const_iterator it =
          scenes.begin();
      it != scenes.end(); ++it)
    rv += (*it)->active_diffuse_sound_fields;
  return rv;
}

uint32_t TASCAR::session_t::get_total_diffuse_sound_fields() const
{
  uint32_t rv(0);
  for(std::vector<TASCAR::scene_render_rt_t*>::const_iterator it =
          scenes.begin();
      it != scenes.end(); ++it)
    rv += (*it)->total_diffuse_sound_fields;
  return rv;
}

TASCAR::range_t::range_t(tsccfg::node_t xmlsrc)
    : xml_element_t(xmlsrc), name(""), start(0), end(0)
{
  GET_ATTRIBUTE(name, "", "range name");
  GET_ATTRIBUTE(start, "s", "start time");
  GET_ATTRIBUTE(end, "s", "end time");
}

TASCAR::connection_t::connection_t(tsccfg::node_t xmlsrc)
    : xml_element_t(xmlsrc), failonerror(false)
{
  GET_ATTRIBUTE(src, "", "jack source port");
  GET_ATTRIBUTE(dest, "", "jack destination port");
  GET_ATTRIBUTE_BOOL(
      failonerror,
      "create an error if connection failed, alternatively just warn");
}

TASCAR::module_base_t::module_base_t(const TASCAR::module_cfg_t& cfg)
    : xml_element_t(cfg.xmlsrc), licensed_component_t(typeid(*this).name()),
      session(cfg.session)
{
}

void TASCAR::module_base_t::update(uint32_t, bool) {}

TASCAR::module_base_t::~module_base_t() {}

std::vector<TASCAR::named_object_t>
TASCAR::session_t::find_objects(const std::string& pattern)
{
  std::vector<TASCAR::named_object_t> retv;
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator sit = scenes.begin();
      sit != scenes.end(); ++sit) {
    std::vector<TASCAR::Scene::object_t*> objs((*sit)->get_objects());
    std::string base("/" + (*sit)->name + "/");
    for(std::vector<TASCAR::Scene::object_t*>::iterator it = objs.begin();
        it != objs.end(); ++it) {
      std::string name(base + (*it)->get_name());
      if(TASCAR::fnmatch(pattern.c_str(), name.c_str(), true) == 0)
        retv.push_back(TASCAR::named_object_t(*it, name));
    }
  }
  return retv;
}

std::vector<TASCAR::named_object_t>
TASCAR::session_t::find_objects(const std::vector<std::string>& vpattern)
{
  std::vector<TASCAR::named_object_t> retv;
  for(const auto& pattern : vpattern) {
    for(auto scene : scenes) {
      std::vector<TASCAR::Scene::object_t*> objs(scene->get_objects());
      std::string base("/" + scene->name + "/");
      for(auto obj : objs) {
        std::string name(base + obj->get_name());
        if(TASCAR::fnmatch(pattern.c_str(), name.c_str(), true) == 0)
          retv.push_back(TASCAR::named_object_t(obj, name));
      }
    }
  }
  return retv;
}

std::vector<TASCAR::Scene::audio_port_t*>
TASCAR::session_t::find_audio_ports(const std::vector<std::string>& pattern)
{
  std::vector<TASCAR::Scene::audio_port_t*> all_ports;
  // first get all audio ports from scenes:
  for(std::vector<TASCAR::scene_render_rt_t*>::iterator sit = scenes.begin();
      sit != scenes.end(); ++sit) {
    std::vector<TASCAR::Scene::object_t*> objs((*sit)->get_objects());
    // std::string base("/"+(*sit)->name+"/");
    for(std::vector<TASCAR::Scene::object_t*>::iterator it = objs.begin();
        it != objs.end(); ++it) {
      // check if this object is derived from audio_port_t:
      TASCAR::Scene::audio_port_t* p_ap(
          dynamic_cast<TASCAR::Scene::audio_port_t*>(*it));
      if(p_ap)
        all_ports.push_back(p_ap);
      // If this is a source, then check sound vertices:
      TASCAR::Scene::src_object_t* p_src(
          dynamic_cast<TASCAR::Scene::src_object_t*>(*it));
      if(p_src) {
        for(std::vector<TASCAR::Scene::sound_t*>::iterator it =
                p_src->sound.begin();
            it != p_src->sound.end(); ++it) {
          TASCAR::Scene::audio_port_t* p_ap(
              dynamic_cast<TASCAR::Scene::audio_port_t*>(*it));
          if(p_ap)
            all_ports.push_back(p_ap);
        }
      }
    }
  }
  // now test for all modules which implement audio_port_t:
  for(std::vector<TASCAR::module_t*>::iterator it = modules.begin();
      it != modules.end(); ++it) {
    TASCAR::Scene::audio_port_t* p_ap(
        dynamic_cast<TASCAR::Scene::audio_port_t*>((*it)->libdata));
    if(p_ap)
      all_ports.push_back(p_ap);
  }
  std::vector<TASCAR::Scene::audio_port_t*> retv;
  // first, iterate over all pattern elements:
  for(auto i_pattern = pattern.begin(); i_pattern != pattern.end();
      ++i_pattern) {
    for(auto p_ap = all_ports.begin(); p_ap != all_ports.end(); ++p_ap) {
      // check if name is matching:
      std::string name((*p_ap)->get_ctlname());
      if((TASCAR::fnmatch(i_pattern->c_str(), name.c_str(), true) == 0) ||
         (*i_pattern == "*"))
        retv.push_back(*p_ap);
    }
  }
  return retv;
}

std::vector<TASCAR::Scene::audio_port_t*>
TASCAR::session_t::find_route_ports(const std::vector<std::string>& pattern)
{
  std::vector<TASCAR::Scene::audio_port_t*> all_ports;
  // now test for all modules which implement audio_port_t:
  for(auto mod : modules) {
    TASCAR::Scene::audio_port_t* p_ap(
        dynamic_cast<TASCAR::Scene::audio_port_t*>(mod->libdata));
    if(p_ap)
      all_ports.push_back(p_ap);
  }
  std::vector<TASCAR::Scene::audio_port_t*> retv;
  // first, iterate over all pattern elements:
  for(const auto& pat : pattern) {
    for(auto p_ap : all_ports) {
      // check if name is matching:
      std::string name(p_ap->get_ctlname());
      if((TASCAR::fnmatch(pat.c_str(), name.c_str(), true) == 0) ||
         (pat == "*"))
        retv.push_back(p_ap);
    }
  }
  return retv;
}

TASCAR::actor_module_t::actor_module_t(const TASCAR::module_cfg_t& cfg,
                                       bool fail_on_empty)
    : module_base_t(cfg)
{
  GET_ATTRIBUTE(actor, "", "pattern to match actor objects");
  obj = session->find_objects(actor);
  if(fail_on_empty && obj.empty())
    throw TASCAR::ErrMsg("No object matches actor pattern \"" +
                         vecstr2str(actor) + "\".");
}

TASCAR::actor_module_t::~actor_module_t() {}

void TASCAR::actor_module_t::set_location(const TASCAR::pos_t& l, bool b_local)
{
  for(std::vector<TASCAR::named_object_t>::iterator it = obj.begin();
      it != obj.end(); ++it) {
    if(b_local) {
      TASCAR::zyx_euler_t o(it->obj->get_orientation());
      TASCAR::pos_t p(l);
      p *= o;
      it->obj->dlocation = p;
    } else {
      it->obj->dlocation = l;
    }
  }
}

void TASCAR::actor_module_t::set_orientation(const TASCAR::zyx_euler_t& o)
{
  for(std::vector<TASCAR::named_object_t>::iterator it = obj.begin();
      it != obj.end(); ++it)
    it->obj->dorientation = o;
}

void TASCAR::actor_module_t::set_transformation(const TASCAR::c6dof_t& tf,
                                                bool b_local)
{
  set_orientation(tf.orientation);
  set_location(tf.position, b_local);
}

void TASCAR::actor_module_t::add_location(const TASCAR::pos_t& l, bool b_local)
{
  for(std::vector<TASCAR::named_object_t>::iterator it = obj.begin();
      it != obj.end(); ++it) {
    if(b_local) {
      TASCAR::zyx_euler_t o(it->obj->get_orientation());
      TASCAR::pos_t p(l);
      p *= o;
      it->obj->dlocation += p;
    } else {
      it->obj->dlocation += l;
    }
  }
}

void TASCAR::actor_module_t::add_orientation(const TASCAR::zyx_euler_t& o)
{
  for(std::vector<TASCAR::named_object_t>::iterator it = obj.begin();
      it != obj.end(); ++it)
    it->obj->dorientation += o;
}

void TASCAR::actor_module_t::add_transformation(const TASCAR::c6dof_t& tf,
                                                bool b_local)
{
  add_orientation(tf.orientation);
  add_location(tf.position, b_local);
}

namespace OSCSession {

  int _addtime(const char*, const char* types, lo_arg** argv, int argc,
               lo_message, void* user_data)
  {
    if((argc == 1) && (types[0] == 'f')) {
      double cur_time(std::max(
          0.0, std::min(((TASCAR::session_t*)user_data)->duration,
                        ((TASCAR::session_t*)user_data)->tp_get_time())));
      ((TASCAR::session_t*)user_data)->tp_locate(cur_time + argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _locate(const char*, const char* types, lo_arg** argv, int argc,
              lo_message, void* user_data)
  {
    if((argc == 1) && (types[0] == 'f')) {
      ((TASCAR::session_t*)user_data)->tp_locate(argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _locatei(const char*, const char* types, lo_arg** argv, int argc,
               lo_message, void* user_data)
  {
    if((argc == 1) && (types[0] == 'i')) {
      ((TASCAR::session_t*)user_data)->tp_locate((uint32_t)(argv[0]->i));
      return 0;
    }
    return 1;
  }

  int _stop(const char*, const char*, lo_arg**, int argc, lo_message,
            void* user_data)
  {
    if(argc == 0) {
      ((TASCAR::session_t*)user_data)->tp_stop();
      return 0;
    }
    return 1;
  }

  int _start(const char*, const char*, lo_arg**, int argc, lo_message,
             void* user_data)
  {
    if(argc == 0) {
      ((TASCAR::session_t*)user_data)->tp_start();
      return 0;
    }
    return 1;
  }

  int _playrange(const char*, const char* types, lo_arg** argv, int argc,
                 lo_message, void* user_data)
  {
    if((argc == 2) && (types[0] == 'f') && (types[1] == 'f')) {
      ((TASCAR::session_t*)user_data)->tp_playrange(argv[0]->f, argv[1]->f);
      return 0;
    }
    return 1;
  }

  int _unload_modules(const char*, const char*, lo_arg**, int argc, lo_message,
                      void* user_data)
  {
    if(argc == 0) {
      ((TASCAR::session_t*)user_data)->unload_modules();
      return 0;
    }
    return 0;
  }

  int _osc_send_xml(const char*, const char* types, lo_arg** argv, int argc,
                    lo_message, void* user_data)
  {
    if(user_data && (argc == 2) && (types[0] == 's') && (types[1] == 's')) {
      TASCAR::session_t* srv(reinterpret_cast<TASCAR::session_t*>(user_data));
      srv->send_xml(&(argv[0]->s), &(argv[1]->s));
    }
    return 0;
  }

} // namespace OSCSession

void TASCAR::session_t::add_transport_methods()
{
  osc_server_t::add_method("/sendxmlto", "ss", OSCSession::_osc_send_xml, this);
  osc_server_t::add_method("/transport/locate", "f", OSCSession::_locate, this);
  osc_server_t::add_method("/transport/locatei", "i", OSCSession::_locatei,
                           this);
  osc_server_t::add_method("/transport/addtime", "f", OSCSession::_addtime,
                           this);
  osc_server_t::add_method("/transport/start", "", OSCSession::_start, this);
  osc_server_t::add_method("/transport/playrange", "ff", OSCSession::_playrange,
                           this);
  osc_server_t::add_method("/transport/stop", "", OSCSession::_stop, this);
  osc_server_t::add_method("/transport/unload", "", OSCSession::_unload_modules,
                           this);
}

void TASCAR::session_t::send_xml(const std::string& url,
                                 const std::string& path)
{
  lo_address target = lo_address_new_from_url(url.c_str());
  if(!target)
    return;
  std::string xml = save_to_string();
  lo_send(target, path.c_str(), "s", xml.c_str());
  lo_address_free(target);
}

void TASCAR::session_t::validate_attributes(std::string& msg) const
{
  root.validate_attributes(msg);
  for(std::vector<TASCAR::scene_render_rt_t*>::const_iterator it =
          scenes.begin();
      it != scenes.end(); ++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::range_t*>::const_iterator it = ranges.begin();
      it != ranges.end(); ++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::connection_t*>::const_iterator it =
          connections.begin();
      it != connections.end(); ++it)
    (*it)->validate_attributes(msg);
  for(std::vector<TASCAR::module_t*>::const_iterator it = modules.begin();
      it != modules.end(); ++it)
    (*it)->validate_attributes(msg);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
