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
#include "acousticmodel.h"

class door_t : public TASCAR::sourcemod_base_t, private TASCAR::Acousticmodel::diffractor_t {
public:
  class data_t : public TASCAR::sourcemod_base_t::data_t {
  public:
    data_t();
    double w;
  };
  door_t(tsccfg::node_t xmlsrc);
  bool read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  TASCAR::sourcemod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure() { n_channels = 1; };
private:
  double width;
  double height;
  double falloff;
  double distance;
  bool wndsqrt;
};

door_t::door_t(tsccfg::node_t xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc),
    width(1),
    height(2),
    falloff(1),
    distance(1),
    wndsqrt(false)
{
  GET_ATTRIBUTE(width,"m","Door width");
  GET_ATTRIBUTE(height,"m","Door height");
  GET_ATTRIBUTE(falloff,"m","Distance at which the gain attenuation starts");
  GET_ATTRIBUTE(distance,"m","Distance by which the source is shifted behind the door");
  GET_ATTRIBUTE_BOOL(wndsqrt,"Flag to control von-Hann fall-off (false, default) or square-root of von-Hann fall-off");
  nonrt_set_rect( width, height );
  diffractor_t::operator+=( TASCAR::pos_t(0,-0.5*width,-0.5*height));
}

bool door_t::read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t* sd )
{
  data_t* d((data_t*)sd);
  double dist(prel.x);
  prel -= nearest(prel);
  TASCAR::pos_t preln(prel.normal());
  double gain(std::max(0.0,preln.x));
  // calculate gain:
  // gain rule: normal hanning window or sqrt
  double gainfalloff(0.5-0.5*cos(TASCAR_PI*std::min(1.0,std::max(0.0,dist)/falloff)));
  if( wndsqrt )
    gainfalloff = sqrt( gainfalloff );
  gain *= gainfalloff;
  double dw((gain-d->w)*t_inc);
  // end gain calc
  preln *= distance;
  prel += preln;
  uint32_t N(output.size());
  float* pi(input[0].d);
  for(uint32_t k=0;k<N;++k){
    output[k] = (*pi)*(d->w+=dw);
    ++pi;
  }
  return true;
}

door_t::data_t::data_t()
  : w(0)
{
}

TASCAR::sourcemod_base_t::data_t* door_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t();
}

REGISTER_SOURCEMOD(door_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
