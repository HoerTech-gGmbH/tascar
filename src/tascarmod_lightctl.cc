#include "session.h"
#include "dmxdriver.h"
#include "serviceclass.h"
#include <unistd.h>

class lightscene_t : public TASCAR::xml_element_t {
public:
  enum method_t {
    nearest, raisedcosine
  };
  lightscene_t( const TASCAR::module_cfg_t& cfg );
  void update(uint32_t frame,bool running);
  void configure(double srate,uint32_t fragsize);
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
  method_t method_;
  // dmx basics:
public:
  std::vector<uint16_t> dmxaddr;
  std::vector<uint16_t> dmxdata;
private:
  std::vector<float> tmpdmxdata;
  std::vector<float> basedmx;
  std::vector<float> objval;
  std::vector<float> objw;
  std::vector<std::vector<float> > fixturevals;
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
  GET_ATTRIBUTE(method);
  GET_ATTRIBUTE(objval);
  GET_ATTRIBUTE(objw);
  if( method.empty() || (method == "nearest") )
    method_ = nearest;
  else if( method == "raisedcosine" )
    method_ = raisedcosine;
  else
    throw TASCAR::ErrMsg("Invalid light render method \""+method+"\" (nearest or raisedcosine).");
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
  fixturevals.resize(fixtures.size());
  for(uint32_t k=0;k<fixtures.size();++k){
    fixturevals[k].resize(channels);
    uint32_t startaddr(1);
    fixtures[k].get_attribute("addr",startaddr);
    std::vector<int32_t> lampdmx;
    fixtures[k].get_attribute("dmxval",lampdmx);
    lampdmx.resize(channels);
    for(uint32_t c=0;c<channels;++c){
      dmxaddr[channels*k+c] = (startaddr+c-1);
      basedmx[channels*k+c] = lampdmx[c];
    }
  }
  objval.resize(channels*objects_.size());
  objw.resize(objects_.size());
  for(uint32_t k=0;k<objw.size();++k)
    objw[k] = 1.0f;
  //
}

void lightscene_t::update(uint32_t frame,bool running)
{
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    tmpdmxdata[k] = 0;
  switch( method_ ){
  case nearest :
    if( parent_.obj ){
      TASCAR::pos_t self_pos(parent_.obj->get_location());
      TASCAR::zyx_euler_t self_rot(parent_.obj->get_orientation());
      // do panning / copy data
      for(uint32_t kobj=0;kobj<objects_.size();++kobj){
        TASCAR::pos_t pobj(objects_[kobj].obj->get_location());
        pobj -= self_pos;
        pobj /= self_rot;
        //float dist(pobj.norm());
        pobj.normalize();
        double dmax(-1);
        double kmax(0);
        for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
          // cos(az) = (pobj * pfixture)
          double caz(dot_prod(pobj,fixtures[kfix].unitvector));
          if( caz > dmax ){
            kmax = kfix;
            dmax = caz;
          }
        }
        for(uint32_t c=0;c<channels;++c)
          tmpdmxdata[channels*kmax+c] += objval[channels*kobj+c];
        }
    }
    break;
  case raisedcosine :
    if( parent_.obj ){
      TASCAR::pos_t self_pos(parent_.obj->get_location());
      TASCAR::zyx_euler_t self_rot(parent_.obj->get_orientation());
      // do panning / copy data
      for(uint32_t kobj=0;kobj<objects_.size();++kobj){
        TASCAR::pos_t pobj(objects_[kobj].obj->get_location());
        pobj -= self_pos;
        pobj /= self_rot;
        //float dist(pobj.norm());
        pobj.normalize();
        for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
          // cos(az) = (pobj * pfixture)
          float caz(dot_prod(pobj,fixtures[kfix].unitvector));
          // 
          float w(powf(0.5f*(caz+1.0f),2.0f/objw[kobj]));
          for(uint32_t c=0;c<channels;++c)
            tmpdmxdata[channels*kfix+c] += w*objval[channels*kobj+c];
        }
      }
    }
    break;
  }
  for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
    for(uint32_t c=0;c<channels;++c)
      tmpdmxdata[channels*kfix+c] += fixturevals[kfix][c];
  }
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    dmxdata[k] = std::min(255.0f,std::max(0.0f,master*tmpdmxdata[k]+basedmx[k]));
}

void lightscene_t::configure(double srate,uint32_t fragsize)
{
}

void lightscene_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_float( "/master", &master );
  if( objval.size() )
    srv->add_vector_float( "/dmx", &objval );
  if( objw.size() )
    srv->add_vector_float( "/w", &objw );
  if( basedmx.size() )
    srv->add_vector_float( "/basedmx", &basedmx );
  for(uint32_t k=0;k<fixturevals.size();++k){
    char ctmp[256];
    sprintf(ctmp,"/fixture%d",k);
    srv->add_vector_float( ctmp, &(fixturevals[k]) );
  }
}

class lightctl_t : public TASCAR::module_base_t, public TASCAR::service_t {
public:
  lightctl_t( const TASCAR::module_cfg_t& cfg );
  ~lightctl_t();
  void update(uint32_t frame,bool running);
  void configure(double srate,uint32_t fragsize);
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
    driver_ = new DMX::OpenDMX_USB_t( device.c_str() );
  }else{
    throw TASCAR::ErrMsg("Unknown DMX driver type \""+driver+"\" (must be \"artnet\" or \"opendmxusb\").");
  }
  add_variables( session );
  start_service();
}

void lightctl_t::update(uint32_t frame,bool running)
{
  for( std::vector<lightscene_t*>::iterator it=lightscenes.begin();it!=lightscenes.end();++it)
    (*it)->update( frame, running );
}

void lightctl_t::configure(double srate,uint32_t fragsize)
{
  for( std::vector<lightscene_t*>::iterator it=lightscenes.begin();it!=lightscenes.end();++it)
    (*it)->configure( srate, fragsize );
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
    driver_->send(universe,localdata);
    usleep( waitusec );
  }
  for(uint32_t k=0;k<localdata.size();++k)
    localdata[k] = 0;
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

