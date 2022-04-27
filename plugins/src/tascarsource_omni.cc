/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "sourcemod.h"

class omni_t : public TASCAR::sourcemod_base_t {
public:
  omni_t(tsccfg::node_t xmlsrc);
  sourcemod_base_t::data_t* create_state_data(double ,
                                              uint32_t ) const
  {
    return NULL;
  };
};

omni_t::omni_t(tsccfg::node_t xmlsrc) : TASCAR::sourcemod_base_t(xmlsrc) {}

REGISTER_SOURCEMOD(omni_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
