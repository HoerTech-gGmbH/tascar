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
#include "ola.h"
#include "session.h"

class fbc_var_t : public TASCAR::module_base_t {
public:
  fbc_var_t(const TASCAR::module_cfg_t& cfg);
  std::string name = "fbc";
  std::vector<std::string> micports = {"system:capture_1"};
  std::vector<std::string> loudspeakerports = {"system:playback_1",
                                               "system:playback_2"};
};

fbc_var_t::fbc_var_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)
{
  GET_ATTRIBUTE(name, "", "Client name, used for jack and IR file name");
  GET_ATTRIBUTE(micports, "", "Microphone ports");
  GET_ATTRIBUTE(loudspeakerports, "", "Loudspeaker ports");
}

class fbc_mod_t : public fbc_var_t {
public:
  fbc_mod_t(const TASCAR::module_cfg_t& cfg);
  void configure();
  void perform_measurement();
  virtual ~fbc_mod_t();
};

fbc_mod_t::fbc_mod_t(const TASCAR::module_cfg_t& cfg) : fbc_var_t(cfg) {}
void fbc_mod_t::configure() {}
fbc_mod_t::~fbc_mod_t() {}

void fbc_mod_t::perform_measurement()
{
  // create jack client with number of micports inputs and one output:
  
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
