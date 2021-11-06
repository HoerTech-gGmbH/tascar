/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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
#include <fstream>

class fence_t : public TASCAR::actor_module_t {
public:
  fence_t(tsccfg::node_t xmlsrc,TASCAR::session_t* session);
  ~fence_t();
  void update(uint32_t frame, bool running);
private:
  std::string id;
  double r;
  double distmax;
  std::string filename;
  TASCAR::sndfile_t* snd;
  double gain;
};

fence_t::fence_t(tsccfg::node_t xmlsrc,TASCAR::session_t* session)
  : actor_module_t(xmlsrc,session),
    r(1.0),
    distmax(0.1),
    snd(NULL),
    gain(0.0)
{
  GET_ATTRIBUTE_(id);
  if( id.empty() )
    id = "fence";
  GET_ATTRIBUTE_(r);
  GET_ATTRIBUTE_(distmax);
  GET_ATTRIBUTE_(filename);
  session->add_double("/"+id+"/r",&r);
  if( !filename.empty() ){
    snd = new TASCAR::sndfile_t(filename);
  }
}

fence_t::~fence_t()
{
  if( snd )
    delete snd;
}

void fence_t::update(uint32_t tp_frame,bool tp_rolling)
{
  double dist(distmax);
  for(std::vector<TASCAR::named_object_t>::iterator iobj=obj.begin();iobj!=obj.end();++iobj){
    TASCAR::Scene::object_t* obj(iobj->obj);
    double od(obj->dlocation.norm());
    dist = std::max(0.0,std::min(dist,r-od));
    if( od > r )
      obj->dlocation *= (r/od);
    //obj->dorientation = dr;
    //obj->dlocation = dp;
  }
  gain = 0.5+0.5*cos(dist/distmax*TASCAR_PI);
}

REGISTER_MODULE(fence_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
