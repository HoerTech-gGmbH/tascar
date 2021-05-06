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
  static int osc_scalefence(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void scalefence(double scale);
private:
  std::string id;
  std::string importraw;
  std::vector<TASCAR::ngon_t> orig_fence;
};

int fence_t::osc_scalefence(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc==1) && (types[0]=='f') )
    ((fence_t*)user_data)->scalefence( argv[0]->f );
  return 0;
}

void fence_t::scalefence(double scale)
{
}

fence_t::fence_t(tsccfg::node_t xmlsrc,TASCAR::session_t* session)
  : actor_module_t(xmlsrc,session)
{
  GET_ATTRIBUTE_(id);
  if( id.empty() )
    id = "fence";
  GET_ATTRIBUTE_(importraw);
  if( !importraw.empty() ){
    std::ifstream rawmesh(TASCAR::env_expand(importraw).c_str());
    if( !rawmesh.good() )
      throw TASCAR::ErrMsg("Unable to open mesh file \""+TASCAR::env_expand(importraw)+"\".");
    while(!rawmesh.eof() ){
      std::string meshline;
      getline(rawmesh,meshline,'\n');
      if( !meshline.empty() ){
        TASCAR::ngon_t ngon;
        ngon.nonrt_set(TASCAR::str2vecpos(meshline));
        orig_fence.push_back(ngon);
      }
    }
  }
  std::stringstream txtmesh(TASCAR::xml_get_text(xmlsrc,"faces"));
  while(!txtmesh.eof() ){
    std::string meshline;
    getline(txtmesh,meshline,'\n');
    if( !meshline.empty() ){
      TASCAR::ngon_t ngon;
      ngon.nonrt_set(TASCAR::str2vecpos(meshline));
      orig_fence.push_back(ngon);
    }
  }
  session->add_method("/"+id+"/scalefence","ff",&fence_t::osc_scalefence,this);
  scalefence(1.0);
}

fence_t::~fence_t()
{
}

void fence_t::update(uint32_t tp_frame,bool tp_rolling)
{
  for(std::vector<TASCAR::named_object_t>::iterator iobj=obj.begin();iobj!=obj.end();++iobj){
    TASCAR::Scene::object_t* obj(iobj->obj);
    // first find nearest face:
    double dmin(distmin);
    //obj->dorientation = dr;
    //obj->dlocation = dp;
  }
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
