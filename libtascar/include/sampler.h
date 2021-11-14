/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
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
#ifndef SAMPLER_H
#define SAMPLER_H

#include "audiochunks.h"
// In order to compile on Windows, the order of the following two
// includes must not be changed (include winsock2.h before windows.h):
// clang-format off
#include "osc_helper.h"
#include "jackclient.h"
// clang-format on

namespace TASCAR {

  class loop_event_t {
  public:
    loop_event_t();
    loop_event_t(int32_t cnt,float gain);
    bool valid() const;
    void process(wave_t& out_chunk,const wave_t& in_chunk);
    uint32_t tsample;
    int32_t tloop;
    float loopgain;
  };

  class looped_sample_t : public TASCAR::sndfile_t {
  public:
    looped_sample_t(const std::string& fname,uint32_t channel);
    ~looped_sample_t();
    void add(loop_event_t);
    void clear();
    void stop();
    void loop(wave_t& chunk);
  private:
    pthread_mutex_t mutex;
    std::vector<loop_event_t> loop_event;
  };

  class sampler_t : public jackc_t, public TASCAR::osc_server_t {
  public:
    sampler_t(const std::string& jname,const std::string& srv_addr,const std::string& srv_port);
    virtual ~sampler_t();
    int process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut);
    void add_sound(const std::string& sound,double gain=0);
    void open_sounds(const std::string& fname);
    void quit() { b_quit = true;};
    void start();
    void stop();
    void run();
  private:
    static int osc_quit(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_addloop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_stoploop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_clearloop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    std::vector<looped_sample_t*> sounds;
    std::vector<std::string> soundnames;
    bool b_quit;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

