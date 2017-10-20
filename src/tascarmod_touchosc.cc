#include "session.h"
#include "serviceclass.h"
#include <unistd.h>

void operator*=(TASCAR::Scene::rgb_color_t& self, double s)
{
  self.r *= s;
  self.g *= s;
  self.b *= s;
}

void operator-=(TASCAR::Scene::rgb_color_t& self, const TASCAR::Scene::rgb_color_t& o)
{
  self.r -= o.r;
  self.g -= o.g;
  self.b -= o.b;
}

double coldist( TASCAR::Scene::rgb_color_t c1, const TASCAR::Scene::rgb_color_t& c2 )
{
  c1 -= c2;
  return sqrt(c1.r*c1.r + c1.g*c1.g + c1.b*c1.b);
  //return fabs(c1.r) + fabs(c1.g) + fabs(c1.g);
}

std::string col2colname( TASCAR::Scene::rgb_color_t col )
{
  double dmax(std::max(col.r,std::max(col.g,col.b)));
  if( dmax > 0 ){
    col *= 1.0/dmax;
  }else{
    col.r = col.g = col.b = 1.0;
  }
  std::vector<TASCAR::Scene::rgb_color_t> cols;
  std::vector<std::string> names;
  cols.push_back(TASCAR::Scene::rgb_color_t("#ff281c"));
  names.push_back("red");
  cols.push_back(TASCAR::Scene::rgb_color_t("#75cc26"));
  names.push_back("green");
  //cols.push_back(TASCAR::Scene::rgb_color_t("#00c4a8"));
  cols.push_back(TASCAR::Scene::rgb_color_t("#0044a8"));
  names.push_back("blue");
  cols.push_back(TASCAR::Scene::rgb_color_t("#ffed00"));
  names.push_back("yellow");
  cols.push_back(TASCAR::Scene::rgb_color_t("#aa7faa"));
  names.push_back("purple");
  cols.push_back(TASCAR::Scene::rgb_color_t("#b2b2b2"));
  names.push_back("gray");
  cols.push_back(TASCAR::Scene::rgb_color_t("#f9a01c"));
  names.push_back("orange");
  cols.push_back(TASCAR::Scene::rgb_color_t("#826647"));
  names.push_back("brown");
  cols.push_back(TASCAR::Scene::rgb_color_t("#ff05f2"));
  names.push_back("pink");
  double dmin(coldist(cols[0],col));
  uint32_t kmin(0);
  for(uint32_t k=1;k<cols.size();++k){
    cols[k] *= 1.0/std::max(cols[k].r,std::max(cols[k].g,cols[k].b));
    double cd(coldist(cols[k],col));
    if( cd<dmin ){
      dmin = cd;
      kmin = k;
    }
  }
  return names[kmin];
}

class connection_t {
public:
  connection_t( const std::string& host, uint32_t port, uint32_t channels );
  void uploadsession( TASCAR::session_t* session );
  void updatesession( TASCAR::session_t* session );
  void setvaluesession( TASCAR::session_t* session, uint32_t channel, float val );
  ~connection_t();
  uint32_t scene;
private:
  connection_t(const connection_t&);
  lo_address target;
  uint32_t channels;
  std::vector<float> vals;
  std::vector<float> levels;
};

connection_t::connection_t( const std::string& host, uint32_t port, uint32_t channels_ )
  : scene(0),
    channels(channels_)
{
  vals.resize(channels);
  levels.resize(channels);
  char ctmp[32];
  sprintf(ctmp,"%d",port);
  target = lo_address_new(host.c_str(),ctmp);
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+host+"\".");
  lo_address_set_ttl(target,1);
}

connection_t::~connection_t()
{
  lo_address_free( target );
}

void connection_t::uploadsession( TASCAR::session_t* session )
{
  uint32_t ch(0);
  if( scene < session->scenes.size() ){
    lo_send(target,"/touchosc/scene","s",session->scenes[scene]->name.c_str());
    for(std::vector<TASCAR::Scene::sound_t*>::iterator it=session->scenes[scene]->sounds.begin();it!=session->scenes[scene]->sounds.end();++it){
      if( ch < channels ){
        char cfader[1024];
        sprintf(cfader,"/touchosc/fader%d",ch+1);
        char clabel[1024];
        sprintf(clabel,"/touchosc/label%d",ch+1);
        char clevel[1024];
        sprintf(clevel,"/touchosc/level%d",ch+1);
        float v((*it)->get_gain_db());
        vals[ch] = v;
        float l((*it)->read_meter());
        if( !(l > 0.0f) )
          l = 0.0f;
        levels[ch] = l;
        lo_send(target,cfader,"f",v);
        lo_send(target,clabel,"s",(*it)->get_parent_name().c_str());
        lo_send(target,clevel,"f",l);
        std::string col(col2colname((*it)->get_color()));
        sprintf(cfader,"/touchosc/fader%d/color",ch+1);
        lo_send(target,cfader,"s",col.c_str());
        sprintf(clabel,"/touchosc/label%d/color",ch+1);
        lo_send(target,clabel,"s",col.c_str());
        ++ch;
      }
    }
    for(std::vector<TASCAR::Scene::receivermod_object_t*>::iterator it=session->scenes[scene]->receivermod_objects.begin();it!=session->scenes[scene]->receivermod_objects.end();++it){
      if( ch < channels ){
        char cfader[1024];
        sprintf(cfader,"/touchosc/fader%d",ch+1);
        char clabel[1024];
        sprintf(clabel,"/touchosc/label%d",ch+1);
        char clevel[1024];
        sprintf(clevel,"/touchosc/level%d",ch+1);
        float v((*it)->get_gain_db());
        vals[ch] = v;
        float l((*it)->read_meter_max());
        if( !(l > 0.0f) )
          l = 0.0f;
        levels[ch] = l;
        lo_send(target,cfader,"f",v);
        lo_send(target,clabel,"s",(*it)->get_name().c_str());
        lo_send(target,clevel,"f",l);
        std::string col(col2colname((*it)->color));
        sprintf(cfader,"/touchosc/fader%d/color",ch+1);
        lo_send(target,cfader,"s",col.c_str());
        sprintf(clabel,"/touchosc/label%d/color",ch+1);
        lo_send(target,clabel,"s",col.c_str());
        ++ch;
      }
    }
  }
  for(uint32_t k=ch;k<channels;++k){
    char cfader[1024];
    sprintf(cfader,"/touchosc/fader%d",k+1);
    char clabel[1024];
    sprintf(clabel,"/touchosc/label%d",k+1);
    char clevel[1024];
    sprintf(clevel,"/touchosc/level%d",k+1);
    lo_send(target,clabel,"s","");
    lo_send(target,cfader,"f",-30.0f);
    lo_send(target,clevel,"f",0.0f);
    sprintf(cfader,"/touchosc/fader%d/color",k+1);
    lo_send(target,cfader,"s","brown");
    sprintf(clabel,"/touchosc/label%d/color",k+1);
    lo_send(target,clabel,"s","brown");
  }
}

void connection_t::updatesession( TASCAR::session_t* session )
{
  uint32_t ch(0);
  if( scene < session->scenes.size() ){
    for(std::vector<TASCAR::Scene::sound_t*>::iterator it=session->scenes[scene]->sounds.begin();it!=session->scenes[scene]->sounds.end();++it){
      if( ch < channels ){
        char cfader[1024];
        sprintf(cfader,"/touchosc/fader%d",ch+1);
        char clevel[1024];
        sprintf(clevel,"/touchosc/level%d",ch+1);
        float v((*it)->get_gain_db());
        if( vals[ch] != v )
          lo_send(target,cfader,"f",v);
        vals[ch] = v;
        float l((*it)->read_meter());
        if( !(l > 0.0f) )
          l = 0.0f;
        if( fabsf(levels[ch] - l) > 0.1){
          lo_send(target,clevel,"f",l);
          levels[ch] = l;
        }
        ++ch;
      }
    }
    for(std::vector<TASCAR::Scene::receivermod_object_t*>::iterator it=session->scenes[scene]->receivermod_objects.begin();it!=session->scenes[scene]->receivermod_objects.end();++it){
      if( ch < channels ){
        char cfader[1024];
        sprintf(cfader,"/touchosc/fader%d",ch+1);
        char clevel[1024];
        sprintf(clevel,"/touchosc/level%d",ch+1);
        float v((*it)->get_gain_db());
        if( vals[ch] != v )
          lo_send(target,cfader,"f",v);
        vals[ch] = v;
        float l((*it)->read_meter_max());
        if( !(l > 0.0f) )
          l = 0.0f;
        if( fabsf(levels[ch] - l) > 0.1){
          lo_send(target,clevel,"f",l);
          levels[ch] = l;
        }
        ++ch;
      }
    }
  }
}

void connection_t::setvaluesession( TASCAR::session_t* session, uint32_t channel, float val )
{
  if( scene < session->scenes.size() ){
    if( channel < session->scenes[scene]->sounds.size() )
      session->scenes[scene]->sounds[channel]->set_gain_db( val );
    else if( channel < session->scenes[scene]->sounds.size()+session->scenes[scene]->receivermod_objects.size() )
      session->scenes[scene]->receivermod_objects[channel-session->scenes[scene]->sounds.size()]->set_gain_db( val );
    else{
      char cfader[1024];
      sprintf(cfader,"/touchosc/fader%d",channel+1);
      lo_send(target,cfader,"f",-30.0f);
    }
  }
  vals[channel] = val;
}

class touchosc_t : public TASCAR::module_base_t, TASCAR::service_t {
public:
  class chref_t {
  public:
    touchosc_t* t;
    uint32_t c;
  };
  touchosc_t( const TASCAR::module_cfg_t& cfg );
  ~touchosc_t();
  void write_xml();
  //virtual void prepare( chunk_cfg_t& );
  //void update(uint32_t frame, bool running);
  static int osc_connect(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_setfader(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_sceneinc(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_scenedec(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void connect(const std::string& host, uint32_t channels);
  void setfader(const std::string& host, uint32_t channel, float val );
  void setscene(const std::string& host, int32_t dscene );
  void service();
private:
  std::string url;
  std::string pattern;
  uint32_t ttl;
  lo_address target;
  std::vector<TASCAR::named_object_t> obj;
  std::vector<lo_message> vmsg;
  std::vector<lo_arg**> vargv;
  std::vector<std::string> vpath;
  std::map<std::string,connection_t*> connections;
  std::vector<chref_t> chrefs;
  pthread_mutex_t mtx;
};

int touchosc_t::osc_connect(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  lo_address src(lo_message_get_source(msg));
  touchosc_t* h((touchosc_t*)user_data);
  h->connect( lo_address_get_hostname(src), argv[0]->i );
  return 0;
}

int touchosc_t::osc_setfader(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  lo_address src(lo_message_get_source(msg));
  touchosc_t::chref_t* h((touchosc_t::chref_t*)user_data);
  h->t->setfader( lo_address_get_hostname(src), h->c, argv[0]->f );
  return 0;
}

int touchosc_t::osc_sceneinc(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  lo_address src(lo_message_get_source(msg));
  touchosc_t* h((touchosc_t*)user_data);
  h->setscene( lo_address_get_hostname(src), 1 );
}

int touchosc_t::osc_scenedec(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  lo_address src(lo_message_get_source(msg));
  touchosc_t* h((touchosc_t*)user_data);
  h->setscene( lo_address_get_hostname(src), -1 );
}

void touchosc_t::setscene(const std::string& host, int32_t dscene )
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    std::map<std::string,connection_t*>::iterator it;
    if( (it=connections.find( host )) != connections.end() ){
      if( it->second ){
        if( ((int32_t)(it->second->scene)+dscene >= 0) && ((int32_t)(it->second->scene)+dscene < session->scenes.size()) ){
          it->second->scene+=dscene;
          it->second->uploadsession(session);
        }
      }
    }
    pthread_mutex_unlock( &mtx );
  }
}

void touchosc_t::setfader(const std::string& host, uint32_t channel, float val )
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    std::map<std::string,connection_t*>::iterator it;
    if( (it=connections.find( host )) != connections.end() ){
      if( it->second )
        it->second->setvaluesession( session, channel, val );
    }
    pthread_mutex_unlock( &mtx );
  }
}

void touchosc_t::connect( const std::string& host, uint32_t channels )
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    std::map<std::string,connection_t*>::iterator it;
    if( (it=connections.find( host )) != connections.end() ){
      delete it->second;
      it->second = NULL;
    }
    connection_t* con(new connection_t(host,9000,channels));
    connections[host] = con;
    con->uploadsession(session);
    pthread_mutex_unlock( &mtx );
  }
}

touchosc_t::touchosc_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    ttl(1)
{
  pthread_mutex_init( &mtx, NULL );
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(pattern);
  GET_ATTRIBUTE(ttl);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  if( pattern.empty() )
    pattern = "/*/*";
  target = lo_address_new_from_url(url.c_str());
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+url+"\".");
  lo_address_set_ttl(target,ttl);
  obj = session->find_objects(pattern);
  if( !obj.size() )
    throw TASCAR::ErrMsg("No target objects found (target pattern: \""+pattern+"\").");
  session->add_method("/touchosc/connect","i",&touchosc_t::osc_connect,this);
  session->add_method("/touchosc/incscene","f",&touchosc_t::osc_sceneinc,this);
  session->add_method("/touchosc/decscene","f",&touchosc_t::osc_scenedec,this);
  chrefs.resize(16);
  for(std::vector<chref_t>::iterator it=chrefs.begin();it!=chrefs.end();++it){
    it->t = this;
    it->c = it-chrefs.begin();
    char ctmp[1024];
    sprintf(ctmp,"/touchosc/fader%d",it->c+1);
    session->add_method(ctmp,"f",&touchosc_t::osc_setfader,&(*it));
  }
  start_service();
}

void touchosc_t::service()
{
  while( run_service ){
    usleep(100000);
    if( pthread_mutex_lock( &mtx ) == 0 ){
      for(std::map<std::string,connection_t*>::iterator it=connections.begin();
          it!=connections.end();++it)
        if( it->second )
          it->second->updatesession( session );
      pthread_mutex_unlock( &mtx );
    }
  }
}

//void touchosc_t::prepare( chunk_cfg_t& cf_ )
//{
//  module_base_t::prepare( cf_ );
//  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
//    vmsg.push_back(lo_message_new());
//    for(uint32_t k=0;k<it->obj->metercnt();++k)
//      lo_message_add_float(vmsg.back(),0);
//    vargv.push_back(lo_message_get_argv(vmsg.back()));
//    vpath.push_back(std::string("/level/")+it->obj->get_name());
//  }
//}

void touchosc_t::write_xml()
{
  SET_ATTRIBUTE(url);
  SET_ATTRIBUTE(pattern);
  SET_ATTRIBUTE(ttl);
}

touchosc_t::~touchosc_t()
{
  stop_service();
  for(std::map<std::string,connection_t*>::iterator it=connections.begin();
      it!=connections.end();++it)
    if( it->second )
      delete it->second;
  for(uint32_t k=0;k<vmsg.size();++k)
    lo_message_free(vmsg[k]);
  lo_address_free(target);
  pthread_mutex_destroy( &mtx );
}

//void touchosc_t::update(uint32_t tp_frame, bool tp_rolling)
//{
//  uint32_t k=0;
//  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
//    const std::vector<float>& leveldata(it->obj->readmeter());
//    uint32_t n(it->obj->metercnt());
//    for(uint32_t km=0;km<n;++km)
//      vargv[k][km]->f = it->obj->get_meterval(km);
//    lo_send_message( target, vpath[k].c_str(), vmsg[k] );
//    ++k;
//  }
//}

REGISTER_MODULE(touchosc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
