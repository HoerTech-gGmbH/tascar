/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include "maskplugin.h"

using namespace TASCAR;

class identity_t : public maskplugin_base_t {
public:
  identity_t(const maskplugin_cfg_t& cfg) : maskplugin_base_t(cfg){};
  float get_gain(const pos_t& pos) { return pos.normal().x; };
  void get_diff_gain(float* gm)
  {
    memset(gm,0,sizeof(float)*16);
    gm[0] = 1.0f;
    gm[3] = 1.0f;
    gm[12] = 1.0f;
    gm[15] = 1.0f;
  };
};

REGISTER_MASKPLUGIN(identity_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
