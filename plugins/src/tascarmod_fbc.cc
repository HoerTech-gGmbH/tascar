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

#include "jackclient.h"
#include "jackiowav.h"
#include "ola.h"
#include "session.h"

class fbc_var_t : public TASCAR::module_base_t {
public:
  fbc_var_t(const TASCAR::module_cfg_t& cfg);
  std::string name = "fbc";
  std::vector<std::string> micports = {"system:capture_1"};
  std::vector<std::string> loudspeakerports = {"system:playback_1",
                                               "system:playback_2"};
  size_t irlen = 44100;
  size_t nrep = 2;
  float level = 60;
};

fbc_var_t::fbc_var_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)
{
  GET_ATTRIBUTE(name, "", "Client name, used for jack and IR file name");
  GET_ATTRIBUTE(micports, "", "Microphone ports");
  GET_ATTRIBUTE(loudspeakerports, "", "Loudspeaker ports");
  GET_ATTRIBUTE(irlen, "samples",
                "Length of impulse response during measurement");
  GET_ATTRIBUTE(level,"dB SPL","Playback level");
  GET_ATTRIBUTE(nrep,"","Number of measurement repetitions");
}

class fbc_mod_t : public fbc_var_t {
public:
  fbc_mod_t(const TASCAR::module_cfg_t& cfg);
  void configure();
  void perform_measurement();
  virtual ~fbc_mod_t();
};

fbc_mod_t::fbc_mod_t(const TASCAR::module_cfg_t& cfg) : fbc_var_t(cfg) {}

void fbc_mod_t::configure()
{
  perform_measurement();
}

fbc_mod_t::~fbc_mod_t() {}

void fbc_mod_t::perform_measurement()
{
  // create jack client with number of micports inputs and one output:
  std::vector<TASCAR::wave_t> isig = {TASCAR::wave_t(irlen * (nrep + 1))};
  TASCAR::fft_t fft(irlen);
  const std::complex<float> If = {0.0f, 1.0f};
  for(size_t k = 0; k < fft.s.n_; k++)
    fft.s[k] = std::exp(-If * TASCAR_2PIf * (float)irlen *
                        std::pow((float)k / (float)(fft.s.n_), 2.0f)) *
               1.0f / sqrtf(irlen);
  fft.ifft();
  fft.w *= TASCAR::db2lin(level - fft.w.spldb());
  for(size_t k = 0; k < nrep + 1; ++k)
    isig[0].append(fft.w);
  std::vector<TASCAR::wave_t> osig;
  for(size_t ch = 0; ch < micports.size(); ++ch)
    osig.push_back(TASCAR::wave_t(irlen * (nrep + 1)));
  for(auto& port : loudspeakerports) {
    std::vector<std::string> ports = {port};
    ports.insert(ports.end(), micports.begin(), micports.end());
    jackio_t jio(isig, osig, ports, name, 0, false);
    jio.run();
  }
}

REGISTER_MODULE(fbc_mod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
