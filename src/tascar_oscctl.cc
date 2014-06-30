#include "osc_helper.h"
#include "jackclient.h"
#include "osc_helper.h"
#include "coordinates.h"
#include <getopt.h>

static bool b_quit(false);

class oscctl_t : public jackc_t, public TASCAR::osc_server_t {
public:
  oscctl_t(const std::string& desturl,const std::string& destpath,const std::string& srv_addr,const std::string& srv_port,const std::string& jackname);
  void run();
  int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
private:
  lo_address dest_addr;
  std::string dest_path;
  float frametime;
  TASCAR::pos_t location;
  TASCAR::zyx_euler_t orientation;
  float step_x_inc;
  float step_x_dec;
  float step_y_inc;
  float step_y_dec;
  float rot_z_inc;
  float rot_z_dec;
  float reset;
  float velocity;
  float ang_velocity;
};

oscctl_t::oscctl_t(const std::string& desturl,const std::string& destpath,const std::string& srv_addr,const std::string& srv_port,const std::string& jackname)
  : jackc_t(jackname),
    osc_server_t(srv_addr,srv_port),
    dest_addr(lo_address_new_from_url(desturl.c_str())),
    dest_path(destpath),
    frametime((double)(get_fragsize())/(double)(get_srate())),
    step_x_inc(0),
    step_x_dec(0),
    step_y_inc(0),
    step_y_dec(0),
    rot_z_inc(0),
    rot_z_dec(0),
    reset(0),
    velocity(1.3),
    ang_velocity(60)
{
  add_float("/1/push5",&reset);
  add_float("/1/push4",&step_x_inc);
  add_float("/1/push6",&step_x_dec);
  add_float("/1/push2",&step_y_inc);
  add_float("/1/push8",&step_y_dec);
  add_float("/1/push10",&rot_z_inc);
  add_float("/1/push12",&rot_z_dec);
}

void oscctl_t::run()
{
  osc_server_t::activate();
  jackc_t::activate();
  while( !b_quit ){
    usleep(10000);
  }
  jackc_t::deactivate();
  osc_server_t::deactivate();
}

int oscctl_t::process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&)
{
  if( reset ){
    orientation = TASCAR::zyx_euler_t();
    location = TASCAR::pos_t();
    reset = 0;
  }
  TASCAR::pos_t dpos;
  TASCAR::zyx_euler_t dori;
  dpos.x = velocity*(step_x_inc - step_x_dec)*frametime;
  dpos.y = velocity*(step_y_inc - step_y_dec)*frametime;
  dori.z = DEG2RAD*ang_velocity*(rot_z_inc - rot_z_dec)*frametime;
  orientation += dori;
  dpos *= orientation;
  location += dpos;
  lo_send(dest_addr,dest_path.c_str(),"ffffff",
          location.x,location.y,location.z,
          RAD2DEG*orientation.z,RAD2DEG*orientation.y,RAD2DEG*orientation.x);
  return 0;
}

static void sighandler(int sig)
{
  b_quit = true;
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_oscctl [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  try{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    std::string destpath("/scene/out/pos");
    std::string desturl("osc.udp://localhost:9877/");
    std::string srv_addr("239.255.1.7");
    std::string srv_port("9877");
    std::string jackname("oscctl");
    const char *options = "hj:p:a:d:u:";
    struct option long_options[] = { 
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'j' },
      { "srvaddr",  1, 0, 'a' },
      { "srvport",  1, 0, 'p' },
      { "destpath", 1, 0, 'd' },
      { "desturl",  1, 0, 'u' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'h':
        usage(long_options);
        return -1;
      case 'j':
        jackname = optarg;
        break;
      case 'p':
        srv_port = optarg;
        break;
      case 'a':
        srv_addr = optarg;
        break;
      case 'd':
        destpath = optarg;
        break;
      case 'u':
        desturl = optarg;
        break;
      }
    }
    oscctl_t O(desturl,destpath,srv_addr,srv_port,jackname);
    O.run();
  }
  catch( const std::exception& msg ){
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch( const char* msg ){
    std::cerr << "Error: " << msg << std::endl;
    return 1;
  }
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
