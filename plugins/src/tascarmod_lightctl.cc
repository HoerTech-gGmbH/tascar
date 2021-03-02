#include "session.h"
#include "dmxdriver.h"
#include "serviceclass.h"
#include <unistd.h>
#include <cmath>

const std::complex<double> i(0.0, 1.0);

double hue_warp_x(0);
double hue_warp_y(0);
double hue_warp_rot(0);

enum method_t {
  nearest, raisedcosine, sawleft, sawright, rect, truncated
};

inline float sqrf( float x )
{
  return x*x;
}

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
  case 4:
    return rect;
  case 5:
    return truncated;
  default:
    return nearest;
  }
}


int osc_set_hsv(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data  ){
    std::vector<float> *data((std::vector<float> *)user_data);
    if( (argc==4) && (argc <= (int)(data->size())) ){
      std::complex<float> cz(std::exp(i*(argv[0]->f*DEG2RAD)));
      cz += hue_warp_x+i*hue_warp_y;
      cz *= std::exp(i*hue_warp_rot);
      float h(std::arg(cz)*RAD2DEG);
      if( h < 0 )
        h += 360.0;
      h = fmodf(h,360.0f);
      float s(argv[1]->f);
      float v(argv[2]->f);
      float d(argv[3]->f);
      float c(v*s);
      float x(c*(1.0f-fabsf(fmodf(h/60.0f,2.0f)-1.0f)));
      float m(v-c);
      TASCAR::pos_t rgb_d;
      if( h < 60 )
        rgb_d = TASCAR::pos_t(c,x,0);
      else if( h < 120 )
        rgb_d = TASCAR::pos_t(x,c,0);
      else if( h < 180 )
        rgb_d = TASCAR::pos_t(0,c,x);
      else if( h < 240 )
        rgb_d = TASCAR::pos_t(0,x,c);
      else if( h < 300 )
        rgb_d = TASCAR::pos_t(x,0,c);
      else
        rgb_d = TASCAR::pos_t(c,0,x);
      (*data)[0] = (rgb_d.x+m)*255.0;
      (*data)[1] = (rgb_d.y+m)*255.0;
      (*data)[2] = (rgb_d.z+m)*255.0;
      (*data)[data->size()-1] = d;
      if( data->size() > (size_t)argc )
        (*data)[3] = m*255.0;
    }
  }
  return 0;
}

method_t string2method( const std::string& m )
{
#define ADDMETHOD(x) if(m==#x) return x
  ADDMETHOD(nearest);
  ADDMETHOD(raisedcosine);
  ADDMETHOD(sawleft);
  ADDMETHOD(sawright);
  ADDMETHOD(rect);
  ADDMETHOD(truncated);
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
  std::vector<TASCAR::table1_t> calibtab;
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
  return 0;
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
  calibtab.resize(n);
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
    if( t_fade <= 0 )
      for(uint32_t c=0;c<n;++c)
        dmx[c] = fade[c];
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
  virtual void validate_attributes(std::string&) const;
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
  bool mixmax;
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
  std::vector<std::string> labels;
  bool usecalib;
  bool sendsquared;
};

void lightscene_t::validate_attributes(std::string& msg) const
{
  TASCAR::xml_element_t::validate_attributes(msg);
  fixtures.validate_attributes(msg);
}

lightscene_t::lightscene_t( const TASCAR::module_cfg_t& cfg )
  : xml_element_t( cfg.xmlsrc ),
    session( cfg.session),
    name("lightscene"),
    fixtures(e,false,"fixture"),
    channels(3),
    master(1),
    mixmax(false),
    parent_(NULL,""),
    usecalib(true),
    sendsquared(false)
{
  GET_ATTRIBUTE_(name);
  GET_ATTRIBUTE_(objects);
  GET_ATTRIBUTE_(parent);
  GET_ATTRIBUTE_(channels);
  GET_ATTRIBUTE_(master);
  GET_ATTRIBUTE_BOOL_(usecalib);
  GET_ATTRIBUTE_BOOL_(sendsquared);
  GET_ATTRIBUTE_BOOL_(mixmax);
  std::string method;
  method_t method_(nearest);
  GET_ATTRIBUTE_(method);
  std::vector<float> tmpobjval;
  get_attribute("objval", tmpobjval, "", "DMX value of objects" );
  std::vector<float> tmpobjw;
  get_attribute("objw", tmpobjw,  "", "weight of objects" );
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
  labels.resize(fixtures.size());
  for(uint32_t k=0;k<fixtures.size();++k){
    fixtureval[k].resize(channels);
    uint32_t startaddr(1);
    fixtures[k].get_attribute("addr",startaddr,"","start address");
    fixtures[k].get_attribute("label",labels[k],"","fixture label");
    std::vector<int32_t> lampdmx;
    fixtures[k].get_attribute("dmxval",lampdmx,"","start DMX value");
    xmlpp::DomParser domp;
    xmlpp::Element* e_calibfile(fixtures[k].e);
    if( fixtures[k].has_attribute("calibfile") ){
      std::string calibfile;
      fixtures[k].get_attribute("calibfile",calibfile,"","calibration file");
      if( calibfile.empty() )
        throw TASCAR::ErrMsg("No speaker calibfile file provided.");
      try{
        domp.parse_file( TASCAR::env_expand( calibfile ) );
      }
      catch( const xmlpp::internal_error& e){
        throw TASCAR::ErrMsg(std::string("xml internal error: ")+e.what());
      }
      catch( const xmlpp::validity_error& e){
        throw TASCAR::ErrMsg(std::string("xml validity error: ")+e.what());
      }
      catch( const xmlpp::parse_error& e){
        throw TASCAR::ErrMsg(std::string("xml parse error: ")+e.what());
      }
      if( !domp )
        throw TASCAR::ErrMsg("Unable to parse file \""+calibfile+"\".");
      e_calibfile = domp.get_document()->get_root_node();
      if( !e_calibfile )
        throw TASCAR::ErrMsg("No root node found in document \""+calibfile+"\".");
      if( e_calibfile->get_name() != "fixturecalib" )
        throw TASCAR::ErrMsg("Invalid root node name. Expected \"fixturecalib\", got "+e_calibfile->get_name()+".");
      xmlpp::Node::NodeList ch(fixtures[k].e->get_children( "calib" ));
      if( !ch.empty() )
        TASCAR::add_warning("calib entries found in calib file and fixture entry.",fixtures[k].e);
    }
    xmlpp::Node::NodeList ch = e_calibfile->get_children( "calib" );
    for( xmlpp::Node::NodeList::iterator it=ch.begin();it!=ch.end();++it){
      xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(*it);
      if( el ){
        int32_t c(-1);
        double v_in(-1);
        double v_out(-1);
        get_attribute_value(el,"channel",c);
        get_attribute_value(el,"in",v_in);
        get_attribute_value(el,"out",v_out);
        if( (c >= 0) && (v_in > 0) && (v_out >= 0) && (c < (int32_t)channels) ){
          //v_out /= v_in;
          fixtureval[k].calibtab[c][v_in] = v_out;
        }
      }
    }
    for(uint32_t c=0;c<channels;++c){
      if( fixtureval[k].calibtab[c].empty() ){
        //fixtureval[k].calibtab[c][0] = 1.0;
        fixtureval[k].calibtab[c][0] = 0.0;
        fixtureval[k].calibtab[c][255] = 255.0;
      }
    }
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
      case truncated :
        {
          // do panning / copy data
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            float caz(dot_prod(pobj,fixtures[kfix].unitvector));
            float w(2.0f*std::max(0.0f,powf(0.5f*(caz+1.0f),2.0f/objval[kobj].w)-0.5f));
            for(uint32_t c=0;c<channels;++c)
              tmpdmxdata[channels*kfix+c] += w*objval[kobj].dmx[c];
          }
        }
        break;
      case sawleft :
        {
          // do panning / copy data
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            float az(objval[kobj].w*std::arg((pobj.x+i*pobj.y)*(fixtures[kfix].unitvector.x-i*fixtures[kfix].unitvector.y)));
            float w(0);
            if( az >= 0 )
              w = 1.0f-std::min(1.0f,az);
            for(uint32_t c=0;c<channels;++c)
              tmpdmxdata[channels*kfix+c] += w*objval[kobj].dmx[c];
          }
        }
        break;
      case sawright :
        {
          // do panning / copy data
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            float az(-objval[kobj].w*std::arg((pobj.x+i*pobj.y)*(fixtures[kfix].unitvector.x-i*fixtures[kfix].unitvector.y)));
            float w(0);
            if( az >= 0 )
              w = 1.0f-std::min(1.0f,az);
            for(uint32_t c=0;c<channels;++c)
              tmpdmxdata[channels*kfix+c] += w*objval[kobj].dmx[c];
          }
        }
        break;
      case rect :
        {
          // do panning / copy data
          for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
            float caz(dot_prod(pobj,fixtures[kfix].unitvector));
            float w(powf(0.5f*(caz+1.0f),2.0f/objval[kobj].w)>0.5);
            for(uint32_t c=0;c<channels;++c)
              tmpdmxdata[channels*kfix+c] += w*objval[kobj].dmx[c];
          }
        }
        break;
      }
    }
  }
  for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
    if( mixmax ){
      float sum_obj(0.0);
      float sum_fix(0.0);
      for(uint32_t c=0;c<channels;++c){
        sum_obj += tmpdmxdata[channels*kfix+c];
        sum_fix += fixtureval[kfix].dmx[c];
      }
      double wmax(std::max(sum_obj,sum_fix));
      if( wmax > 0 ){
        sum_obj /= wmax;
        sum_fix /= wmax;
      }
      for(uint32_t c=0;c<channels;++c){
        tmpdmxdata[channels*kfix+c] *= sum_obj;
        tmpdmxdata[channels*kfix+c] += sum_fix*fixtureval[kfix].dmx[c];
      }
    }else{
      for(uint32_t c=0;c<channels;++c){
        tmpdmxdata[channels*kfix+c] += fixtureval[kfix].dmx[c];
      }
    }
    if( usecalib )
      for(uint32_t c=0;c<channels;++c){
        tmpdmxdata[channels*kfix+c] = fixtureval[kfix].calibtab[c].interp(tmpdmxdata[channels*kfix+c]);
      }
  }
  if( sendsquared )
    for(uint32_t k=0;k<tmpdmxdata.size();++k){
      float v(255.0f*sqrf(0.0039215686274509803377*(master*tmpdmxdata[k]+basedmx[k])));
      if( v < 1.0f )
        v = 0.0f;
      dmxdata[k] = std::min(255.0f,std::max(0.0f,ceilf(v)));
    }
  else
    for(uint32_t k=0;k<tmpdmxdata.size();++k)
      dmxdata[k] = std::min(255.0f,std::max(0.0f,master*tmpdmxdata[k]+basedmx[k]));
}

void lightscene_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_bool("/usecalib", &usecalib );
  srv->add_bool("/sendsquared", &sendsquared );
  srv->add_float( "/master", &master );
  srv->add_bool( "/mixmax", &mixmax );
  if( basedmx.size() )
    srv->add_vector_float( "/basedmx", &basedmx );
  for( uint32_t ko=0;ko<objects_.size();++ko){
    srv->add_vector_float( "/"+objects_[ko].obj->get_name()+"/dmx", &(objval[ko].dmx) );
    srv->add_vector_float( "/"+objects_[ko].obj->get_name()+"/fade", &(objval[ko].fade) );
    srv->add_float( "/"+objects_[ko].obj->get_name()+"/w", &(objval[ko].w) );
    srv->add_vector_float( "/"+objects_[ko].obj->get_name()+"/wfade", &(objval[ko].wfade) );
    srv->add_method( "/"+objects_[ko].obj->get_name()+"/method", "i", osc_setmethod, &(objval[ko].method));
    if( channels >= 3 )
      srv->add_method( "/"+objects_[ko].obj->get_name()+"/hsv", "ffff", osc_set_hsv, &(objval[ko].fade) );
  }
  for(uint32_t k=0;k<fixtureval.size();++k){
    std::string label(labels[k]);
    if( label.empty() ){
      char ctmp[256];
      sprintf(ctmp,"fixture%d",k);
      label = ctmp;
    }
    label = "/" + label + "/";
    srv->add_vector_float( label+"dmx", &(fixtureval[k].dmx) );
    srv->add_vector_float( label+"fade", &(fixtureval[k].fade) );
    if( channels >= 3 )
      srv->add_method( label+"hsv", "ffff", osc_set_hsv, &(fixtureval[k].fade) );
  }
}

class lightctl_t : public TASCAR::module_base_t, public TASCAR::service_t {
public:
  lightctl_t( const TASCAR::module_cfg_t& cfg );
  ~lightctl_t();
  void update(uint32_t frame,bool running);
  void add_variables( TASCAR::osc_server_t* srv );
  virtual void validate_attributes(std::string&) const;
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
  GET_ATTRIBUTE_(fps);
  GET_ATTRIBUTE_(universe);
  GET_ATTRIBUTE_(priority);
  GET_ATTRIBUTE_(driver);
  if( driver == "artnetdmx" ){
    std::string hostname;
    std::string port;
    GET_ATTRIBUTE_(hostname);
    if( hostname.empty() )
      hostname = "localhost";
    GET_ATTRIBUTE_(port);
    if( port.empty() )
      port = "6454";
    driver_ = new DMX::ArtnetDMX_t( hostname.c_str(), port.c_str() );
  }else if( driver == "opendmxusb" ){
    std::string device;
    GET_ATTRIBUTE_(device);
    if( device.empty() )
      device = "/dev/ttyUSB0";
    try{
      driver_ = new DMX::OpenDMX_USB_t( device.c_str() );
    }
    catch( const std::exception& e ){
      driver_ = NULL;
      TASCAR::add_warning("Unable to open DMX USB driver " + device + ".");
    }
  }else if( driver == "osc" ){
    std::string hostname;
    std::string port;
    GET_ATTRIBUTE_(hostname);
    if( hostname.empty() )
      hostname = "localhost";
    GET_ATTRIBUTE_(port);
    if( port.empty() )
      port = "9000";
    uint32_t maxchannels(512);
    GET_ATTRIBUTE_(maxchannels);
    driver_ = new DMX::OSC_t( hostname.c_str(), port.c_str(), maxchannels );
  }else{
    throw TASCAR::ErrMsg("Unknown DMX driver type \""+driver+"\" (must be \"artnetdmx\", \"osc\" or \"opendmxusb\").");
  }
  add_variables( session );
  start_service();
  GET_ATTRIBUTE_(hue_warp_x);
  GET_ATTRIBUTE_(hue_warp_y);
  GET_ATTRIBUTE_DEG_(hue_warp_rot);
}

void lightctl_t::validate_attributes(std::string& msg) const
{
  TASCAR::module_base_t::validate_attributes(msg);
  for( auto it=lightscenes.begin();it!=lightscenes.end();++it)
    (*it)->validate_attributes(msg);
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
  srv->add_double( "/light/hue_warp_x", &hue_warp_x );
  srv->add_double( "/light/hue_warp_y", &hue_warp_y );
  srv->add_double_degree( "/light/hue_warp_rot", &hue_warp_rot );
}

lightctl_t::~lightctl_t()
{
  stop_service();
  usleep(100000);
  for( auto it=lightscenes.begin();it!=lightscenes.end();++it)
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

