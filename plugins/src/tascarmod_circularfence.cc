#include "session.h"
#include <fstream>

class fence_t : public TASCAR::actor_module_t {
public:
  fence_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
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

fence_t::fence_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
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
  gain = 0.5+0.5*cos(dist/distmax*M_PI);
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
