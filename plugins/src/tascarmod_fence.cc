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
