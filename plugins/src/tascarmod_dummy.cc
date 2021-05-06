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

#include "session.h"

class dummy_t : public TASCAR::actor_module_t {
public:
  dummy_t( const TASCAR::module_cfg_t& cfg );
  ~dummy_t();
  void update(uint32_t frame, bool running);
  void configure( );
  void release();
private:
  bool checkprepare;
  bool localprep;
};

dummy_t::dummy_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg ),checkprepare(false),localprep(false)
{
  GET_ATTRIBUTE_BOOL_(checkprepare);
  DEBUG(1);
  DEBUG(f_sample);
  DEBUG(f_fragment);
  DEBUG(t_sample);
  DEBUG(t_fragment);
  DEBUG(n_fragment);
}

void dummy_t::release()
{
  DEBUG(localprep);
  if( checkprepare ){
    if( !localprep )
      throw TASCAR::ErrMsg("not prepared (local)");
    if( !is_prepared() )
      throw TASCAR::ErrMsg("not prepared (base)");
  }
  localprep = false;
  actor_module_t::release();
  DEBUG(localprep);
}

void dummy_t::configure( )
{
  if( checkprepare ){
    if( localprep )
      throw TASCAR::ErrMsg("prepared (local)");
    if( is_prepared() )
      throw TASCAR::ErrMsg("prepared (base)");
  }
  localprep = true;
  actor_module_t::configure();
  DEBUG(1);
  DEBUG(f_sample);
  DEBUG(f_fragment);
  DEBUG(t_sample);
  DEBUG(t_fragment);
  DEBUG(n_fragment);
}

dummy_t::~dummy_t()
{
  if( checkprepare ){
    if( localprep )
      std::cerr << "prepared (local)" << std::endl;
    if( is_prepared() )
      std::cerr << "prepared (base)" << std::endl;
  }
  DEBUG(1);
}

void dummy_t::update(uint32_t frame,bool running)
{
  if( checkprepare ){
    if( !localprep )
      throw TASCAR::ErrMsg("not prepared (local)");
    if( !is_prepared() )
      throw TASCAR::ErrMsg("not prepared (base)");
  }
  DEBUG(localprep);
}

REGISTER_MODULE(dummy_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
