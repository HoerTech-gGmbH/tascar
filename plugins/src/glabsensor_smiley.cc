/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
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

#include "glabsensorplugin.h"

using namespace TASCAR;

class smiley_t : public sensorplugin_drawing_t {
public:
  smiley_t( const TASCAR::sensorplugin_cfg_t& cfg );
  void add_variables( TASCAR::osc_server_t* srv );
  void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
private:
  float happiness;
  bool hide;
};

smiley_t::smiley_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_drawing_t(cfg),
    happiness(1.0),
    hide(false)
{
}

void smiley_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_float("/happy",&happiness);
  srv->add_bool("/hide",&hide);
}

void smiley_t::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
  if( !hide ){
    if( happiness < -0.8 )
      add_critical( "!$&!//&!" );
    else if( happiness < 0.1 )
      add_warning( "I am not happy." );
    float s(std::min(width,height));
    cr->arc(0.5*width, 0.5*height, 0.2*s, 0.0, 2 * M_PI);
    cr->stroke();
    cr->arc(0.5*width-0.1*s, 0.5*height-0.05*s, 0.02*s, 0.0, 2 * M_PI);
    cr->arc(0.5*width+0.1*s, 0.5*height-0.05*s, 0.02*s, 0.0, 2 * M_PI);
    cr->fill();
    cr->move_to(0.5*width-0.1*s, 0.5*height+0.1*s-0.05*s*happiness);
    for(uint32_t k=0;k<32;++k){
      float x(((float)k-15.0)/16.0);
      cr->line_to(0.5*width+x*0.1*s,0.5*height+0.1*s-0.05*s*happiness*x*x);
    }
    cr->stroke();
    alive();
  }
}

REGISTER_SENSORPLUGIN(smiley_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
