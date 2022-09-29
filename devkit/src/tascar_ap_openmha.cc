/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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

#include <openmha/mha_algo_comm.hh>
#include <openmha/mhapluginloader.h>
#include <tascar/audioplugin.h>
#include <tascar/coordinates.h>
#include <tascar/errorhandling.h>

class openmha_t : public TASCAR::audioplugin_base_t,
                  public MHA_AC::algo_comm_class_t {
public:
  openmha_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables(TASCAR::osc_server_t* srv);
  static int osc_parse(const char* path, const char* types, lo_arg** argv,
                       int argc, lo_message msg, void* user_data);
  void parse(const std::string& s);
  ~openmha_t();

private:
  std::string plugin;
  std::string config;
  PluginLoader::mhapluginloader_t mhaplug;
  MHASignal::waveform_t* sIn;
  MHA_AC::waveform_t acpos;
  MHA_AC::waveform_t acrot;
};

openmha_t::openmha_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg),
      plugin(tsccfg::node_get_attribute_value(e, "plugin")),
      config(tsccfg::node_get_attribute_value(e, "config")),
      mhaplug(*this, plugin), sIn(NULL), acpos(*this, "pos", 3, 1, true),
      acrot(*this, "rot", 3, 1, true)
{
  GET_ATTRIBUTE(plugin, "", "Plugin name");
  GET_ATTRIBUTE(config, "", "Configuration command handed to openmha");
  if(!config.empty())
    std::cout << mhaplug.parse(config) << std::endl;
  std::stringstream scfg(tsccfg::node_get_text(e, ""));
  while(!scfg.eof()) {
    std::string cfgline;
    getline(scfg, cfgline, '\n');
    MHAParser::trim(cfgline);
    if(!cfgline.empty()) {
      std::cout << mhaplug.parse(cfgline) << std::endl;
    }
  }
}

int openmha_t::osc_parse(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data)
{
  ((openmha_t*)user_data)->parse(&(argv[0]->s));
  return 0;
}

void openmha_t::parse(const std::string& s)
{
  try {
    std::cout << mhaplug.parse(s) << std::endl;
  }
  catch(const std::exception& e) {
    std::string msg("openMHA Error: ");
    msg += e.what();
    TASCAR::add_warning(msg);
  }
}

void openmha_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->add_method("/parse", "s", osc_parse, this);
}

void openmha_t::configure()
{
  audioplugin_base_t::configure();
  mhaconfig_t mhacfg;
  mhacfg.channels = n_channels;
  mhacfg.domain = MHA_WAVEFORM;
  mhacfg.fragsize = n_fragment;
  mhacfg.wndlen = n_fragment;
  mhacfg.fftlen = n_fragment;
  mhacfg.srate = f_sample;
  mhaconfig_t mhacfg_out(mhacfg);
  mhaplug.prepare(mhacfg);
  PluginLoader::mhaconfig_compare(mhacfg_out, mhacfg, "openmha");
  sIn = new MHASignal::waveform_t(n_fragment, n_channels);
}

void openmha_t::release()
{
  mhaplug.release();
  audioplugin_base_t::release();
  delete sIn;
}

openmha_t::~openmha_t() {}

void openmha_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                           const TASCAR::pos_t& pos,
                           const TASCAR::zyx_euler_t& rot,
                           const TASCAR::transport_t& tp)
{
  try {
    acpos.insert();
    acrot.insert();
    // copy sound vertex position:
    acpos.buf[0] = pos.x;
    acpos.buf[1] = pos.y;
    acpos.buf[2] = pos.z;
    // copy object orientation:
    acrot.buf[0] = rot.z;
    acrot.buf[1] = rot.y;
    acrot.buf[2] = rot.x;
    // copy non-interleaved TASCAR data to interleaved openMHA data:
    for(uint32_t k = 0; k < n_channels; ++k)
      chunk[k].copy_to_stride(&(sIn->buf[k]), sIn->num_frames, n_channels);
    // process audio in openMHA:
    mha_wave_t* sOut(NULL);
    mhaplug.process(sIn, &sOut);
    // copy interleaved openMHA data back to non-interleaved TASCAR data:
    for(uint32_t k = 0; k < n_channels; ++k)
      chunk[k].copy_stride(&(sOut->buf[k]), sOut->num_frames, n_channels);
  }
  catch(const std::exception& e) {
    std::cerr << "Error (process): " << e.what() << std::endl;
  }
}

REGISTER_AUDIOPLUGIN(openmha_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
