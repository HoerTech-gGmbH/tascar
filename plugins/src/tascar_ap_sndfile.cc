/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include "audioplugin.h"
#include "levelmeter.h"
#include "errorhandling.h"
#include <fstream>
#include <thread>
#include <mutex>

class ap_sndfile_cfg_t : public TASCAR::audioplugin_base_t {
public:
  ap_sndfile_cfg_t( const TASCAR::audioplugin_cfg_t& cfg );
protected:
  std::string name;
  uint32_t channel;
  double start;
  double position;
  double length;
  uint32_t loop;
  bool resample;
  std::string levelmode;
  TASCAR::levelmeter::weight_t weighting;
  double level;
  bool triggered;
  bool transport;
  bool mute;
  std::string license;
  std::string attribution;
  std::string channelorder;
};

ap_sndfile_cfg_t::ap_sndfile_cfg_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    channel(0),
    start(0),
    position(0),
    length(0),
    loop(1),
    resample(false),
    levelmode("rms"),
    weighting(TASCAR::levelmeter::Z),
    level(0),
    triggered(false),
    transport(true),
    mute(false)
{
  GET_ATTRIBUTE(name,"","Sound file name");
  GET_ATTRIBUTE(channel,"","First sound file channel to be used, zero-base");
  GET_ATTRIBUTE(start,"s","Start position within the file");
  GET_ATTRIBUTE(position,"s","Start position within the scene");
  GET_ATTRIBUTE(length,"s","length of sound sample, or 0 to use whole file length");
  GET_ATTRIBUTE(loop,"","loop count or 0 for infinite looping");
  GET_ATTRIBUTE_BOOL(resample,"Allow resampling to current session sample rate");
  GET_ATTRIBUTE(levelmode,"","level mode, ``rms'', ``peak'' or ``calib''");
  GET_ATTRIBUTE_NOUNIT(weighting,"level weighting for RMS mode");
  GET_ATTRIBUTE_DB(level,"level, meaning depends on \\attr{levelmode}");
  GET_ATTRIBUTE_BOOL(triggered,"Play OSC triggered samples, ignore position and loop");
  GET_ATTRIBUTE_BOOL(transport,"Use session time base");
  GET_ATTRIBUTE_BOOL(mute,"Load muted");
  GET_ATTRIBUTE(channelorder,"","Channel order in case of First Order Ambisonics recordings, ``FuMa'', ``ACN'' or ``none''");
  if( start < 0 )
    throw TASCAR::ErrMsg("file start time must be positive.");
}

class ap_sndfile_t : public ap_sndfile_cfg_t  {
public:
  ap_sndfile_t( const TASCAR::audioplugin_cfg_t& cfg );
  ~ap_sndfile_t();
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  void add_licenses( licensehandler_t* session );
  void load_file();
  void unload_file();
  void configure();
  void release();
private:
  static int osc_loadfile(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void osc_loadfile(const std::string& fname, const std::string& lmode, float level);
  uint32_t triggeredloop;
  TASCAR::transport_t ltp;
  std::vector<TASCAR::sndfile_t*> sndf;
  std::mutex mtx;
};

ap_sndfile_t::ap_sndfile_t(const TASCAR::audioplugin_cfg_t& cfg)
    : ap_sndfile_cfg_t(cfg), triggeredloop(0)
{
  get_license_info(e, name, license, attribution);
}

void ap_sndfile_t::configure()
{
  TASCAR::audioplugin_base_t::configure();
  load_file();
}

void ap_sndfile_t::load_file()
{
  mtx.lock();
  sndf.clear();
  try {
    if(n_channels < 1)
      throw TASCAR::ErrMsg("At least one channel required.");
    if(name.size()) {
      if((n_channels == 4) && (channelorder != "none")) {
        // probably FOA, check for channelorder
        if(channelorder.empty()) {
          TASCAR::add_warning(
              "No channel order is specified, but probably FOA format. Please "
              "specify ``FuMa'', ``ACN'' or ``none''.",
              e);
        }
        if((channelorder == "FuMa") || channelorder.empty()) {
          sndf.push_back(new TASCAR::sndfile_t(name, 0, start, length));
          sndf.push_back(new TASCAR::sndfile_t(name, 2, start, length));
          sndf.push_back(new TASCAR::sndfile_t(name, 3, start, length));
          sndf.push_back(new TASCAR::sndfile_t(name, 1, start, length));
        } else if(channelorder == "ACN") {
          sndf.push_back(new TASCAR::sndfile_t(name, 0, start, length));
          sndf.push_back(new TASCAR::sndfile_t(name, 1, start, length));
          sndf.push_back(new TASCAR::sndfile_t(name, 2, start, length));
          sndf.push_back(new TASCAR::sndfile_t(name, 3, start, length));
        } else
          throw TASCAR::ErrMsg("Invalid channel order: \"" + channelorder +
                               "\"");
      } else {
        for(uint32_t ch = 0; ch < n_channels; ++ch) {
          sndf.push_back(
              new TASCAR::sndfile_t(name, channel + ch, start, length));
        }
      }
      if(sndf[0]->get_srate() != f_sample) {
        double origsrate(sndf[0]->get_srate());
        if(resample) {
          std::vector<std::thread*> threads;
          for(auto sf : sndf) {
            threads.push_back(new std::thread(&TASCAR::sndfile_t::resample, sf,
                                              f_sample / origsrate));
            // sf->resample(f_sample / origsrate);
          }
          for(auto th : threads) {
            th->join();
            delete th;
          }
        } else {
          std::string msg("The sample rate of the sound file " + name +
                          " differs from the audio system sample rate: ");
          char ctmp[1024];
          sprintf(ctmp, "file has %d Hz, expected %g Hz", sndf[0]->get_srate(),
                  f_sample);
          msg += ctmp;
          TASCAR::add_warning(msg, e);
        }
      }
      double gain(1);
      if(levelmode == "rms") {
        TASCAR::levelmeter_t meter(f_sample, sndf[0]->n / f_sample, weighting);
        meter.update(*(sndf[0]));
        gain = level * 2e-5 / meter.rms();
      } else if(levelmode == "peak") {
        float maxabs(0);
        for(auto sf : sndf)
          maxabs = std::max(maxabs, sf->maxabs());
        if(maxabs > 0)
          gain = level * 2e-5 / maxabs;
      } else if(levelmode == "calib")
        gain = level * 2e-5;
      else
        throw TASCAR::ErrMsg("Invalid level mode \"" + levelmode +
                             "\". (sndfile)");
      for(auto sf : sndf) {
        if(triggered) {
          sf->set_position(-(sf->n) * (sf->get_srate()));
          sf->set_loop(1);
        } else {
          sf->set_position(position);
          sf->set_loop(loop);
        }
        *(sf) *= gain;
      }
    }
  }
  catch(...) {
    mtx.unlock();
    throw;
  }
  mtx.unlock();
}

void ap_sndfile_t::release()
{
  TASCAR::audioplugin_base_t::release();
  unload_file();
}

void ap_sndfile_t::unload_file()
{
  mtx.lock();
  for(auto it = sndf.begin(); it != sndf.end(); ++it)
    delete(*it);
  sndf.clear();
  mtx.unlock();
}

ap_sndfile_t::~ap_sndfile_t()
{
}

void ap_sndfile_t::add_licenses( licensehandler_t* session )
{
  audioplugin_base_t::add_licenses( session );
  if( name.size() )
  session->add_license( license, attribution, TASCAR::tscbasename(TASCAR::env_expand(name)) );
}

int ap_sndfile_t::osc_loadfile(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data  )
    ((ap_sndfile_t*)user_data)->osc_loadfile(&(argv[0]->s),&(argv[1]->s),argv[2]->f);
  return 0;
}

void ap_sndfile_t::osc_loadfile(const std::string& fname,
                                const std::string& lmode, float nlevel)
{
  mtx.lock();
  name = fname;
  levelmode = lmode;
  level = powf(10.0f, 0.05f * nlevel);
  mtx.unlock();
  try {
    unload_file();
    load_file();
  }
  catch(const std::exception& e) {
    TASCAR::add_warning(std::string("Error while loading file: ") + e.what());
  }
}

void ap_sndfile_t::add_variables( TASCAR::osc_server_t* srv )
{
  if( triggered )
    srv->add_uint( "/loop", &triggeredloop );
  else
    srv->add_uint( "/loop", &loop );
  srv->add_bool( "/mute", &mute );
  srv->add_method("/loadfile","ssf",&osc_loadfile,this);
  srv->add_double("/position", &position);
}

void ap_sndfile_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                              const TASCAR::pos_t& pos,
                              const TASCAR::zyx_euler_t&,
                              const TASCAR::transport_t& tp)
{
  if(mtx.try_lock()) {
    if(sndf.size()) {
      if(transport)
        ltp = tp;
      if(triggered) {
        if(triggeredloop) {
          for(auto sf : sndf) {
            sf->set_iposition(ltp.object_time_samples);
            sf->set_loop(triggeredloop);
          }
          triggeredloop = 0;
        }
      }
      if((!mute) && (tp.rolling || (!transport))) {
        for(uint32_t ch = 0; ch < std::min(sndf.size(), chunk.size()); ++ch)
          sndf[ch]->add_to_chunk(ltp.object_time_samples, chunk[ch]);
      }
      if(!transport)
        ltp.object_time_samples += chunk[0].n;
    }
    mtx.unlock();
  }
}

REGISTER_AUDIOPLUGIN(ap_sndfile_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
