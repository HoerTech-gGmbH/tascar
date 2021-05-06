/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "session.h"
#include <thread>

class sleep_t : public TASCAR::module_base_t {
public:
  sleep_t(const TASCAR::module_cfg_t& cfg);
  ~sleep_t(){};

private:
  double sleep;
};

sleep_t::sleep_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg), sleep(1)
{
  GET_ATTRIBUTE_(sleep);
  std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000.0 * sleep)));
}

REGISTER_MODULE(sleep_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
