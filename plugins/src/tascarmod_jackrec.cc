#include "jackiowav.h"
#include "session.h"
#include <mutex>

#define OSC_VOID(x)                                                            \
  static int x(const char* path, const char* types, lo_arg** argv, int argc,   \
               lo_message msg, void* user_data)                                \
  {                                                                            \
    ((jackrec_t*)user_data)->x();                                              \
    return 0;                                                                  \
  };                                                                           \
  void x()

#define OSC_STRING(x)                                                          \
  static int x(const char* path, const char* types, lo_arg** argv, int argc,   \
               lo_message msg, void* user_data)                                \
  {                                                                            \
    ((jackrec_t*)user_data)->x(&(argv[0]->s));                                 \
    return 0;                                                                  \
  };                                                                           \
  void x(const std::string&)

class jackrec_t : public TASCAR::module_base_t {
public:
  jackrec_t(const TASCAR::module_cfg_t& cfg);
  ~jackrec_t();
  void add_variables(TASCAR::osc_server_t* srv);
  OSC_VOID(start);
  OSC_VOID(stop);
  OSC_VOID(clearports);
  OSC_STRING(addport);

private:
  std::string name;
  double buflen;
  std::string ofname;
  jackrec_async_t* jr;
  std::mutex mtx;
  std::vector<std::string> ports;
};

jackrec_t::jackrec_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), name("jackrec"), buflen(10), jr(NULL)
{
  // get configuration variables:
  GET_ATTRIBUTE(name);
  GET_ATTRIBUTE(buflen);
  // register OSC variables:
  add_variables(session);
}

jackrec_t::~jackrec_t()
{
  std::lock_guard<std::mutex> lock(mtx);
  if(jr)
    delete jr;
}

void jackrec_t::clearports()
{
  ports.clear();
}

void jackrec_t::addport(const std::string& port)
{
  ports.push_back(port);
}

void jackrec_t::start()
{
  std::lock_guard<std::mutex> lock(mtx);
  if(jr)
    delete jr;
  try {
    std::string ofname_(ofname);
    if(ofname.empty()) {
      time_t rawtime;
      struct tm* timeinfo;
      char buffer[80];
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);
      ofname_ = std::string("rec") + std::string(buffer) + std::string(".wav");
    }
    jr = new jackrec_async_t(ofname_, ports, name, buflen);
  }
  catch(const std::exception& e) {
    jr = NULL;
    TASCAR::add_warning(e.what());
  }
}

void jackrec_t::stop()
{
  std::lock_guard<std::mutex> lock(mtx);
  if(jr)
    delete jr;
  jr = NULL;
}

void jackrec_t::add_variables(TASCAR::osc_server_t* srv)
{
  std::string prefix(srv->get_prefix());
  srv->set_prefix(std::string("/") + name);
  srv->add_string("/name", &ofname);
  srv->add_method("/start", "", &jackrec_t::start, this);
  srv->add_method("/stop", "", &jackrec_t::stop, this);
  srv->add_method("/clear", "", &jackrec_t::clearports, this);
  srv->add_method("/addport", "s", &jackrec_t::addport, this);
  srv->set_prefix(prefix);
}

REGISTER_MODULE(jackrec_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
