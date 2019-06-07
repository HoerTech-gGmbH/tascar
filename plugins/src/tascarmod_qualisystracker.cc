#include "session.h"
#include <mutex>
#include <sys/time.h>

using namespace TASCAR;

double gettime()
{
  struct timeval tv;
  memset(&tv,0,sizeof(timeval));
  gettimeofday(&tv, NULL );
  return (double)(tv.tv_sec) + 0.000001*tv.tv_usec;
}

#define OSCHANDLER(classname,methodname) \
  static int methodname(const char *path, const char *types, lo_arg **argv, \
                        int argc, lo_message msg, void *user_data){ \
    return ((classname*)(user_data))->methodname(path,types,argv,argc,msg);} \
  int methodname(const char *path, const char *types, lo_arg **argv, int argc, \
                 lo_message msg)

class qualisys_tracker_t : public TASCAR::actor_module_t {
public:
  qualisys_tracker_t( const TASCAR::module_cfg_t& cfg );
  ~qualisys_tracker_t();
  void add_variables( TASCAR::osc_server_t* srv );
  void prepare( chunk_cfg_t& );
  void release();
  void update(uint32_t frame, bool running);
  OSCHANDLER(qualisys_tracker_t,qtmres);
  OSCHANDLER(qualisys_tracker_t,qtmxml);
  OSCHANDLER(qualisys_tracker_t,qtm6d);
private:
  std::string qtmurl;
  double timeout;
  std::vector<double> influence;
  bool local;
  bool incremental;
  lo_address qtmtarget;
  std::mutex mtx;
  int32_t srv_port;
  std::string rigid;
  double last_prepared;
  TASCAR::c6dof_t transform;
};

int qualisys_tracker_t::qtmres(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg)
{
  return 0;
}

int qualisys_tracker_t::qtmxml(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg)
{
  lo_send(qtmtarget,"/qtm","sss", "StreamFrames", "AllFrames", "6DEuler" );
  return 0;
}

int qualisys_tracker_t::qtm6d(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg)
{
  if( strncmp(path,"/qtm/6d_euler/",14) == 0 ){
    std::lock_guard<std::mutex> lock(mtx);
    std::string sentrigid(&(path[14]));
    if( rigid == sentrigid ){
      transform.position.x = 0.001*influence[0]*(argv[0]->f);
      transform.position.y = 0.001*influence[1]*(argv[1]->f);
      transform.position.z = 0.001*influence[2]*(argv[2]->f);
      transform.orientation.z = DEG2RAD*influence[3]*(argv[3]->f);
      transform.orientation.y = DEG2RAD*influence[4]*(argv[4]->f);
      transform.orientation.x = DEG2RAD*influence[5]*(argv[5]->f);
    }
  }
  return 0;
}

qualisys_tracker_t::qualisys_tracker_t( const TASCAR::module_cfg_t& cfg )
  : TASCAR::actor_module_t(cfg),
  qtmurl("osc.udp://localhost:22225"),
  timeout(1.0),
  local(false),
  incremental(false),
  srv_port(0),
  last_prepared(gettime())
{
  GET_ATTRIBUTE(qtmurl);
  GET_ATTRIBUTE(timeout);
  GET_ATTRIBUTE(influence);
  GET_ATTRIBUTE_BOOL(local);
  GET_ATTRIBUTE_BOOL(incremental);
  GET_ATTRIBUTE(rigid);
  for(uint32_t k=influence.size();k<6;++k)
    influence.push_back(1.0);
  qtmtarget = lo_address_new_from_url(qtmurl.c_str());
  add_variables( session );
}

void qualisys_tracker_t::update(uint32_t tp_frame,bool tp_rolling)
{
  if( incremental )
    add_transformation( transform, local );
  else
    set_transformation( transform, local );
}


void qualisys_tracker_t::prepare(chunk_cfg_t&)
{
  std::lock_guard<std::mutex> lock(mtx);
  last_prepared = gettime();
  lo_send(qtmtarget,"/qtm","si", "Connect", srv_port );
  lo_send(qtmtarget,"/qtm","ss", "GetParameters", "All" );
}

void qualisys_tracker_t::release()
{
  std::lock_guard<std::mutex> lock(mtx);
  lo_send(qtmtarget,"/qtm","s", "Disconnect" );
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

REGISTER_MODULE(qualisys_tracker_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
