/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "session.h"

class fail_t : public TASCAR::module_base_t {
public:
  fail_t( const TASCAR::module_cfg_t& cfg );
  ~fail_t();
  void update(uint32_t frame, bool running);
  void configure();
  void release();
private:
  bool failinit;
  bool failprepare;
  bool failrelease;
  bool failupdate;
};

fail_t::fail_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    failinit(false),
    failprepare(false),
    failrelease(false),
    failupdate(false)
{
  GET_ATTRIBUTE_BOOL_(failinit);
  GET_ATTRIBUTE_BOOL_(failprepare);
  GET_ATTRIBUTE_BOOL_(failrelease);
  GET_ATTRIBUTE_BOOL_(failupdate);
  if( failinit )
    throw TASCAR::ErrMsg("init.");
}

void fail_t::release()
{
  if( failrelease )
    throw TASCAR::ErrMsg("release.");
  module_base_t::release();
}

void fail_t::configure()
{
  DEBUG(this);
  if( failprepare )
    throw TASCAR::ErrMsg("prepare.");
  module_base_t::configure( );
}

fail_t::~fail_t()
{
}

void fail_t::update(uint32_t, bool)
{
  if(failupdate)
    throw TASCAR::ErrMsg("update.");
}

REGISTER_MODULE(fail_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
