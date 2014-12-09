#include "receivermod.h"

class cardioid_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    float azgain;
    double dt;
  };
  cardioid_t(xmlpp::Element* xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
};

cardioid_t::data_t::data_t(uint32_t chunksize)
{
  azgain = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

cardioid_t::cardioid_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
}

void cardioid_t::add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  data_t* d((data_t*)sd);
  float dazgain((0.5*cos(prel.azim())+0.5 - d->azgain)*dt);
  for(uint32_t k=0;k<chunk.size();k++)
    outchannels[0][k] += chunk[k]*(d->azgain+=dazgain);
}

void cardioid_t::add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  output[0] += chunk.w();
}

uint32_t cardioid_t::get_num_channels()
{
  return 1;
}

TASCAR::receivermod_base_t::data_t* cardioid_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize);
}

REGISTER_RECEIVERMOD(cardioid_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
