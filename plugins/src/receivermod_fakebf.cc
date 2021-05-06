/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "receivermod.h"
#include "delayline.h"

class ortf_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(double srate,uint32_t chunksize,double maxdist,double c,uint32_t sincorder);
    double fs;
    double dt;
    TASCAR::varidelay_t dline_l;
    TASCAR::varidelay_t dline_r;
    double wl;
    double wr;
    double itd;
  };
  ortf_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void configure();
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
private:
  double distance;
  double angle;
  double start_angle;
  double stop_angle;
  uint32_t sincorder;
  double c;
  TASCAR::pos_t dir_l;
  TASCAR::pos_t dir_r;
  TASCAR::pos_t dir_itd;
};

ortf_t::data_t::data_t(double srate,uint32_t chunksize,double maxdist,double c,uint32_t sincorder)
  : fs(srate),
    dt(1.0/std::max(1.0,(double)chunksize)),
    dline_l(2*maxdist*srate/c+2+sincorder,srate,c,sincorder,64),
    dline_r(2*maxdist*srate/c+2+sincorder,srate,c,sincorder,64),
    wl(0),wr(0),itd(0)
{
}

ortf_t::ortf_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    distance(0.17),
    angle(110*DEG2RAD),
    start_angle(0),
    stop_angle(0.5*M_PI),
    sincorder(0),
    c(340),
    dir_l(1,0,0),
    dir_r(1,0,0),
    dir_itd(0,1,0)
{
  GET_ATTRIBUTE(distance,"m","Microphone distance");
  GET_ATTRIBUTE_DEG(angle,"Angular distance between microphone axes");
  GET_ATTRIBUTE_DEG(start_angle,"Angle at which attenutation ramp starts");
  GET_ATTRIBUTE_DEG(stop_angle,"Angle at which full attenutation is reached");
  GET_ATTRIBUTE(sincorder,"","Sinc interpolation order of ITD delay line");
  GET_ATTRIBUTE(c,"m/s","Speed of sound");
  dir_l.rot_z(0.5*angle);
  dir_r.rot_z(-0.5*angle);
}

void ortf_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t prel_norm(prel.normal());
  double start_cos(cos(start_angle));
  double stop_cos(cos(stop_angle));
  // calculate panning parameters (as incremental values):
  double wl(std::max(0.0,std::min(1.0,(dot_prod(prel_norm,dir_l)-stop_cos)/(start_cos-stop_cos))));
  double wr(std::max(0.0,std::min(1.0,(dot_prod(prel_norm,dir_r)-stop_cos)/(start_cos-stop_cos))));
  double dwl((wl - d->wl)*d->dt);
  double dwr((wr - d->wr)*d->dt);
  double ditd((distance*(0.5*dot_prod(prel_norm,dir_itd)+0.5) - d->itd)*d->dt);
  // apply panning:
  uint32_t N(chunk.size());
  for(uint32_t k=0;k<N;++k){
    float v(chunk[k]);
    output[0][k] += d->dline_l.get_dist_push(distance-d->itd,v)*d->wl;
    output[1][k] += d->dline_r.get_dist_push(d->itd,v)*d->wr;
    d->wl+=dwl;
    d->wr+=dwr;
    d->itd += ditd;
  }
}

void ortf_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  float* o_l(output[0].d);
  float* o_r(output[1].d);
  const float* i_w(chunk.w().d);
  const float* i_x(chunk.x().d);
  const float* i_y(chunk.y().d);
  //const float* i_z(chunk.z().d);
  for(uint32_t k=0;k<chunk.size();++k){
    *o_l += *i_w + dir_l.x*(*i_x) + dir_l.y*(*i_y);
    *o_r += *i_w + dir_r.x*(*i_x) + dir_r.y*(*i_y);
    ++o_l;
    ++o_r;
    ++i_w;
    ++i_x;
    ++i_y;
  }
  //output[0] += chunk.w() + chunk.x() + chunk.y();
  //output[1] += chunk.w();
}

void ortf_t::configure()
{
  n_channels = 2;
  labels.clear();
  labels.push_back("_l");
  labels.push_back("_r");
}

TASCAR::receivermod_base_t::data_t* ortf_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(srate,fragsize,distance,c,sincorder);
}

REGISTER_RECEIVERMOD(ortf_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
