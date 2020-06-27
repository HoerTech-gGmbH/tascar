#include "session.h"
#include <thread>

class sleep_t : public TASCAR::module_base_t {
public:
  sleep_t(const TASCAR::module_cfg_t& cfg);
  ~sleep_t(){};

private:
  double sleep;
};

sleep_t::sleep_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg), sleep(1)
{
  GET_ATTRIBUTE(sleep);
  std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000.0 * sleep)));
}

REGISTER_MODULE(sleep_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
