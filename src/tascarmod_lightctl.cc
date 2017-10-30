#include "session.h"
#include "dmxdriver.h"
#include "serviceclass.h"
#include <unistd.h>
#include <complex.h>

enum method_t {
  nearest, raisedcosine, sawleft, sawright
};

method_t uint2method( uint32_t m )
{
  switch( m ){
  case 0:
    return nearest;
  case 1:
    return raisedcosine;
  case 2:
    return sawleft;
  case 3:
    return sawright;
  default:
    return nearest;
  }
}

method_t string2method( const std::string& m )
{
#define ADDMETHOD(x) if(m==#x) return x
  ADDMETHOD(nearest);
  ADDMETHOD(raisedcosine);
  ADDMETHOD(sawleft);
  ADDMETHOD(sawright);
  return nearest;
#undef ADDMETHOD
}

class lobj_t {
public:
  lobj_t();
  void resize( uint32_t channels );
  void update( double t_frame );
  method_t method;
  float w;
  float dw;
  std::vector<float> wfade;
  std::vector<float> dmx;
  std::vector<float> fade;
  std::vector<float> calib;
private:
  uint32_t n;
  double t_fade;
  double t_wfade;
  std::vector<float> ddmx;
};

static int osc_setmethod(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  method_t* m((method_t*)user_data);
  *m = uint2method( argv[0]->i );
}

lobj_t::lobj_t()
  : method(nearest), w(1.0), dw(0.0), n(1), t_fade(0), t_wfade(0)
{
  resize(1);
}

void lobj_t::resize( uint32_t channels )
{
  n = channels;
  wfade.resize(2);
  dmx.resize(n);
  fade.resize(n+1);
  ddmx.resize(n);
  calib.resize(n);
}

void lobj_t::update( double t_frame )
{
  if( wfade[1] > 0 ){
    t_wfade = wfade[1];
    wfade[1] = 0;
    dw = (wfade[0]-w)*t_frame/t_wfade;
  }
  if( fade[n] > 0 ){
    t_fade = fade[n];
    fade[n] = 0;
    for(uint32_t c=0;c<n;++c)
      ddmx[c] = (fade[c]-dmx[c])*t_frame/t_fade;
  }
  if( t_fade > 0 ){
    for(uint32_t c=0;c<n;++c)
      dmx[c] += ddmx[c];
    t_fade -= t_frame;
  }
  if( t_wfade > 0 ){
    w += dw;
    t_wfade -= t_frame;
  }
}

class lightscene_t : public TASCAR::xml_element_t {
public:
  lightscene_t( const TASCAR::module_cfg_t& cfg );
  void update( uint32_t frame, bool running, double t_fragment );
  void add_variables( TASCAR::osc_server_t* srv );
  const std::string& get_name() const {return name;};
private:
  TASCAR::session_t* session;
  // config variables:
  std::string name;
  TASCAR::spk_array_t fixtures;
  uint32_t channels;
  std::string objects;
  std::string parent;
  float master;
  std::string method;
  // derived params:
  TASCAR::named_object_t parent_;
  std::vector<TASCAR::named_object_t> objects_;
  std::vector<float> values;
  //method_t method_;
  // dmx basics:
public:
  std::vector<uint16_t> dmxaddr;
  std::vector<uint16_t> dmxdata;
private:
  std::vector<float> tmpdmxdata;
  std::vector<float> basedmx;
  std::vector<lobj_t> objval;
  std::vector<lobj_t> fixtureval;
};

lightscene_t::lightscene_t( const TASCAR::module_cfg_t& cfg )
  : xml_element_t( cfg.xmlsrc ),
    session( cfg.session),
    name("lightscene"),
    fixtures(e,"fixture"),
    channels(3),
    master(1),
    parent_(NULL,"")
{
  GET_ATTRIBUTE(name);
  GET_ATTRIBUTE(objects);
  GET_ATTRIBUTE(parent);
  GET_ATTRIBUTE(channels);
  GET_ATTRIBUTE(master);
  std::string method;
  method_t method_(nearest);
  GET_ATTRIBUTE(method);
  std::vector<float> tmpobjval;
  get_attribute("objval", tmpobjval );
  std::vector<float> tmpobjw;
  get_attribute("objw", tmpobjw );
  method_ = string2method( method );
  objects_ = session->find_objects( objects );
  std::vector<TASCAR::named_object_t> o(session->find_objects(parent));
  if( o.size()>0)
    parent_ = o[0];
  //
  if( fixtures.size() == 0 )
    throw TASCAR::ErrMsg("No fixtures defined!");
  // allocate data:
  dmxaddr.resize(fixtures.size()*channels);
  dmxdata.resize(fixtures.size()*channels);
  basedmx.resize(fixtures.size()*channels);
  tmpdmxdata.resize(fixtures.size()*channels);
  fixtureval.resize(fixtures.size());
  for(uint32_t k=0;k<fixtures.size();++k){
    fixtureval[k].resize(channels);
    uint32_t startaddr(1);
    fixtures[k].get_attribute("addr",startaddr);
    std::vector<int32_t> lampdmx;
    fixtures[k].get_attribute("dmxval",lampdmx);
    fixtures[k].get_attribute("calib",fixtureval[k].calib);
    for(uint32_t c=fixtureval[k].calib.size();c<channels;++c)
      fixtureval[k].calib[c] = 1.0;
    lampdmx.resize(channels);
    for(uint32_t c=0;c<channels;++c){
      dmxaddr[channels*k+c] = (startaddr+c-1);
      basedmx[channels*k+c] = lampdmx[c];
    }
  }
  objval.resize(objects_.size());
  //
  uint32_t i(0);
  for(uint32_t k=0;k<objects_.size();++k){
    objval[k].resize( channels );
    objval[k].method = method_;
    if( k<tmpobjw.size() )
      objval[k].w = tmpobjw[k];
    if( objval[k].w == 0 )
      objval[k].w = 1.0f;
    for( uint32_t ch=0;ch<channels;++ch){
      if( i<tmpobjval.size() ){
        objval[k].dmx[ch] = tmpobjval[i++];
      }
    }
  }
}

void lightscene_t::update( uint32_t frame, bool running, double t_fragment )
{
  for( std::vector<lobj_t>::iterator it=objval.begin();it!=objval.end();++it)
    it->update( t_fragment );
  for( std::vector<lobj_t>::iterator it=fixtureval.begin();it!=fixtureval.end();++it)
    it->update( t_fragment );
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    tmpdmxdata[k] = 0;
  if( parent_.obj ){
    TASCAR::pos_t self_pos(parent_.obj->get_location());
    TASCAR::zyx_euler_t self_rot(parent_.obj->get_orientation());
    for(uint32_t kobj=0;kobj<objects_.size();++kobj){
      TASCAR::Scene::object_t* obj(objects_[kobj].obj);
      TASCAR::pos_t pobj(obj->get_location());
      pobj -= self_pos;
      pobj /= self_rot;
      pobj.normalize();
      switch( objval[kobj].method ){
      case nearest :
        {
          // do panning / copy data
          double dmax(-1);
          double kmax(0);
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            double caz(dot_prod(pobj,fixtures[kfix].unitvector));
            if( caz > dmax ){
              kmax = kfix;
              dmax = caz;
            }
          }
          for(uint32_t c=0;c<channels;++c)
            tmpdmxdata[channels*kmax+c] += objval[kobj].dmx[c];
        }
        break;
      case raisedcosine :
        {
          // do panning / copy data
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            float caz(dot_prod(pobj,fixtures[kfix].unitvector));
            float w(powf(0.5f*(caz+1.0f),2.0f/objval[kobj].w));
            for(uint32_t c=0;c<channels;++c)
              tmpdmxdata[channels*kfix+c] += w*objval[kobj].dmx[c];
          }
        }
        break;
      case sawleft :
        {
          // do panning / copy data
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            float az(objval[kobj].w*cargf((pobj.x+I*pobj.y)*(fixtures[kfix].unitvector.x-I*fixtures[kfix].unitvector.y)));
            float w(1.0f-std::max(0.0f,std::min(1.0f,az)));
            for(uint32_t c=0;c<channels;++c)
              tmpdmxdata[channels*kfix+c] += w*objval[kobj].dmx[c];
          }
        }
        break;
      case sawright :
        {
          // do panning / copy data
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            float az(objval[kobj].w*cargf((pobj.x+I*pobj.y)*(fixtures[kfix].unitvector.x-I*fixtures[kfix].unitvector.y)));
            float w(1.0f-std::max(0.0f,std::min(1.0f,-az)));
            for(uint32_t c=0;c<channels;++c)
              tmpdmxdata[channels*kfix+c] += w*objval[kobj].dmx[c];
          }
        }
        break;
      }
    }
  }
  for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
    for(uint32_t c=0;c<channels;++c){
      tmpdmxdata[channels*kfix+c] += fixtureval[kfix].dmx[c];
      // calibration:
      tmpdmxdata[channels*kfix+c] *= fixtureval[kfix].calib[c];
    }
  }
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    dmxdata[k] = std::min(255.0f,std::max(0.0f,master*tmpdmxdata[k]+basedmx[k]));
}

void lightscene_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_float( "/master", &master );
  if( basedmx.size() )
    srv->add_vector_float( "/basedmx", &basedmx );
  for( uint32_t ko=0;ko<objects_.size();++ko){
    srv->add_vector_float( "/"+objects_[ko].obj->get_name()+"/dmx", &(objval[ko].dmx) );
    srv->add_vector_float( "/"+objects_[ko].obj->get_name()+"/fade", &(objval[ko].fade) );
    srv->add_float( "/"+objects_[ko].obj->get_name()+"/w", &(objval[ko].w) );
    srv->add_vector_float( "/"+objects_[ko].obj->get_name()+"/wfade", &(objval[ko].wfade) );
    srv->add_method( "/"+objects_[ko].obj->get_name()+"/method", "i", osc_setmethod, &(objval[ko].method));
  }
  for(uint32_t k=0;k<fixtureval.size();++k){
    char ctmp[256];
    sprintf(ctmp,"/fixture%d/dmx",k);
    srv->add_vector_float( ctmp, &(fixtureval[k].dmx) );
    sprintf(ctmp,"/fixture%d/fade",k);
    srv->add_vector_float( ctmp, &(fixtureval[k].fade) );
  }
}

class lightctl_t : public TASCAR::module_base_t, public TASCAR::service_t {
public:
  lightctl_t( const TASCAR::module_cfg_t& cfg );
  ~lightctl_t();
  void update(uint32_t frame,bool running);
  void add_variables( TASCAR::osc_server_t* srv );
  virtual void service();
private:
  std::string driver;
  double fps;
  uint32_t universe;
  // derived params:
  std::vector<lightscene_t*> lightscenes;
  DMX::driver_t *driver_;
};

lightctl_t::lightctl_t( const TASCAR::module_cfg_t& cfg )
  : TASCAR::module_base_t( cfg ),
    fps(30),
    universe(0),
    driver_(NULL)
{
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "lightscene")){
      TASCAR::module_cfg_t lcfg(cfg);
      lcfg.xmlsrc = sne;
      lightscenes.push_back( new lightscene_t( lcfg ) );
    }
  }
  GET_ATTRIBUTE(fps);
  GET_ATTRIBUTE(universe);
  GET_ATTRIBUTE(priority);
  GET_ATTRIBUTE(driver);
  if( driver == "artnetdmx" ){
    std::string hostname;
    std::string port;
    GET_ATTRIBUTE(hostname);
    if( hostname.empty() )
      hostname = "localhost";
    GET_ATTRIBUTE(port);
    if( port.empty() )
      port = "6454";
    driver_ = new DMX::ArtnetDMX_t( hostname.c_str(), port.c_str() );
  }else if( driver == "opendmxusb" ){
    std::string device;
    GET_ATTRIBUTE(device);
    if( device.empty() )
      device = "/dev/ttyUSB0";
    try{
      driver_ = new DMX::OpenDMX_USB_t( device.c_str() );
    }
    catch( const std::exception& e ){
      driver_ = NULL;
      std::cerr << "WARNING: Unable to open DMX USB driver " << device << ".\n";
    }
  }else{
    throw TASCAR::ErrMsg("Unknown DMX driver type \""+driver+"\" (must be \"artnet\" or \"opendmxusb\").");
  }
  add_variables( session );
  start_service();
}

void lightctl_t::update(uint32_t frame,bool running)
{
  for( std::vector<lightscene_t*>::iterator it=lightscenes.begin();it!=lightscenes.end();++it)
    (*it)->update( frame, running, t_fragment );
}

void lightctl_t::add_variables( TASCAR::osc_server_t* srv )
{
  std::string old_prefix(srv->get_prefix());
  for( std::vector<lightscene_t*>::iterator it=lightscenes.begin();it!=lightscenes.end();++it){
    srv->set_prefix(old_prefix + "/light/" + (*it)->get_name());
    (*it)->add_variables( srv );
  }
  srv->set_prefix(old_prefix);
}

lightctl_t::~lightctl_t()
{
  stop_service();
  usleep(100000);
  for( std::vector<lightscene_t*>::iterator it=lightscenes.begin();it!=lightscenes.end();++it)
    delete (*it);
  if( driver_ )
    delete driver_;
}

void lightctl_t::service()
{
  uint32_t waitusec(1000000/fps);
  std::vector<uint16_t> localdata;
  localdata.resize(512);
  usleep( 1000 );
  while( run_service ){
    for(uint32_t k=0;k<localdata.size();++k)
      localdata[k] = 0;
    for( uint32_t kscene=0;kscene<lightscenes.size();++kscene)
      for(uint32_t k=0;k<lightscenes[kscene]->dmxaddr.size();++k)
        localdata[lightscenes[kscene]->dmxaddr[k]] = std::min((uint16_t)255,lightscenes[kscene]->dmxdata[k]);
    if( driver_ )
      driver_->send(universe,localdata);
    usleep( waitusec );
  }
  for(uint32_t k=0;k<localdata.size();++k)
    localdata[k] = 0;
  if( driver_ )
    driver_->send(universe,localdata);
}

REGISTER_MODULE(lightctl_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

