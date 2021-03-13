#include "glabsensorplugin.h"
#include <mutex>
#include <lsl_cpp.h>
#include <sys/time.h>

using namespace TASCAR;

double gettime()
{
  struct timeval tv;
  memset(&tv,0,sizeof(timeval));
  gettimeofday(&tv, NULL );
  return (double)(tv.tv_sec) + 0.000001*tv.tv_usec;
}

class rigid_t {
public:
  rigid_t(const std::string& name, double freq );
  ~rigid_t();
  void process( double x, double y, double z, double rz, double ry, double rx );
private:
  lsl::stream_info lsl_info;
  lsl::stream_outlet lsl_stream;
public:
  std::vector<double> c6dof;
  double last_call;
};

rigid_t::rigid_t(const std::string& name, double freq )
  : lsl_info(name,"MoCap", 6, freq, lsl::cf_float32, std::string("qtm_")+name ),
    lsl_stream(lsl_info),
    c6dof(6,0.0),
    last_call(gettime()-24.0*3600.0)
{
}

rigid_t::~rigid_t()
{
}

void rigid_t::process( double x, double y, double z, double rz, double ry, double rx )
{
  x *= 0.001;
  y *= 0.001;
  z *= 0.001;
  c6dof[0] = x;
  c6dof[1] = y;
  c6dof[2] = z;
  c6dof[3] = DEG2RAD*rz;
  c6dof[4] = DEG2RAD*ry;
  c6dof[5] = DEG2RAD*rx;
  lsl_stream.push_sample( c6dof );
  last_call = gettime();
}

#define OSCHANDLER(classname,methodname) \
  static int methodname(const char *path, const char *types, lo_arg **argv, \
                        int argc, lo_message msg, void *user_data){ \
    return ((classname*)(user_data))->methodname(path,types,argv,argc,msg);} \
  int methodname(const char *path, const char *types, lo_arg **argv, int argc, \
                 lo_message msg)

class qualisys_tracker_t : public sensorplugin_drawing_t {
public:
  qualisys_tracker_t( const TASCAR::sensorplugin_cfg_t& cfg );
  ~qualisys_tracker_t();
  void add_variables( TASCAR::osc_server_t* srv );
  void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
  void prepare();
  void release();
  OSCHANDLER(qualisys_tracker_t,qtmres);
  OSCHANDLER(qualisys_tracker_t,qtmxml);
  OSCHANDLER(qualisys_tracker_t,qtm6d);
private:
  std::string qtmurl;
  double timeout;
  lo_address qtmtarget;
  std::mutex mtx;
  int32_t srv_port;
  double nominal_freq;
  std::map<std::string,rigid_t*> rigids;
  double last_prepared;
};

int qualisys_tracker_t::qtmres(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg)
{
  return 0;
}

int qualisys_tracker_t::qtmxml(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg)
{
  TASCAR::xml_doc_t qtmcfg(&(argv[0]->s), TASCAR::xml_doc_t::LOAD_STRING );
  TASCAR::xml_element_t root(qtmcfg.root);
  nominal_freq = tsccfg::node_xpath_to_number(root.e,"/*/General/Frequency");
  for( auto the6d : tsccfg::node_get_children(root.e,"The_6D")){
    for( auto body : tsccfg::node_get_children(the6d,"Body")){
      for( auto bname : tsccfg::node_get_children(body,"Name")){
        std::string name(tsccfg::node_get_text(bname));
        if( rigids.find(name) != rigids.end() )
          TASCAR::add_warning("Rigid "+name+" was allocated more than once.");
        rigids[name] = new rigid_t(name,nominal_freq);
      }
    }
  }
  lo_send(qtmtarget,"/qtm","sss", "StreamFrames", "AllFrames", "6DEuler" );
  return 0;
}

int qualisys_tracker_t::qtm6d(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg)
{
  if( strncmp(path,"/qtm/6d_euler/",14) == 0 ){
    std::lock_guard<std::mutex> lock(mtx);
    auto it(rigids.find(&(path[14])));
    if( it != rigids.end() ){
      alive();
      it->second->process( argv[0]->f, argv[1]->f, argv[2]->f,
                           argv[3]->f, argv[4]->f, argv[5]->f );
    }
  }
  return 0;
}

qualisys_tracker_t::qualisys_tracker_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_drawing_t(cfg),
    qtmurl("osc.udp://localhost:22225"),
    timeout(1.0),
    srv_port(0),
    nominal_freq(1),
    last_prepared(gettime())
{
  GET_ATTRIBUTE_(qtmurl);
  GET_ATTRIBUTE_(timeout);
  qtmtarget = lo_address_new_from_url(qtmurl.c_str());
}

void qualisys_tracker_t::prepare()
{
  std::lock_guard<std::mutex> lock(mtx);
  last_prepared = gettime();
  for( auto it=rigids.begin();it!=rigids.end();++it)
    delete it->second;
  rigids.clear();
  lo_send(qtmtarget,"/qtm","si", "Connect", srv_port );
  lo_send(qtmtarget,"/qtm","ss", "GetParameters", "All" );
}

void qualisys_tracker_t::release()
{
  std::lock_guard<std::mutex> lock(mtx);
  lo_send(qtmtarget,"/qtm","s", "Disconnect" );
  for( auto it=rigids.begin();it!=rigids.end();++it)
    delete it->second;
  rigids.clear();
}

qualisys_tracker_t::~qualisys_tracker_t()
{
  lo_address_free(qtmtarget);
}

void qualisys_tracker_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv_port = srv->get_srv_port();
  srv->set_prefix("");
  srv->add_method("/qtm/cmd_res","s",&qtmres,this);
  srv->add_method("/qtm/xml","s",&qtmxml,this);
  srv->add_method("","ffffff",&qtm6d,this);
}

void qualisys_tracker_t::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
  double y(0.05);
  bool any_visible(false);
  {
    std::lock_guard<std::mutex> lock(mtx);
    for( auto it = rigids.begin(); it!= rigids.end(); ++it ){
      y += 0.15;
      cr->move_to(0.05*width,y*height);
      cr->show_text(it->first);
      cr->stroke();
      if( gettime() - it->second->last_call < timeout ){
        any_visible = true;
        cr->move_to(0.23*width,y*height);
        char ctmp[256];
        sprintf(ctmp,"%7.3f %7.3f %7.3f m %7.1f %7.1f %7.1f deg",
                it->second->c6dof[0],
                it->second->c6dof[1],
                it->second->c6dof[2],
                it->second->c6dof[3]*RAD2DEG,
                it->second->c6dof[4]*RAD2DEG,
                it->second->c6dof[5]*RAD2DEG);
        cr->show_text(ctmp);
        cr->stroke();
        // rotation:
        cr->save();
        cr->set_line_width(1);
        for(uint32_t c=0;c<3;++c){
          double lx((0.8+0.05*c)*width);
          double ly((y-0.04)*height);
          double r(0.055*height);
          cr->arc(lx, ly, r, 0, PI2 );
          cr->stroke();
          double lrot(DEG2RAD * it->second->c6dof[3+c]);
          cr->move_to(lx,ly);
          cr->line_to(lx+r*cos(lrot),ly+r*sin(lrot));
          cr->stroke();
        }
        cr->restore();
      }
    }
  }
  if( (!any_visible) && (gettime() - last_prepared > 5.0) )
    prepare();
}

REGISTER_SENSORPLUGIN(qualisys_tracker_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
