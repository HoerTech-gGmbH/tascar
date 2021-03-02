#include "receivermod.h"

class cardioid_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    float w;
    double dt;
  };
  cardioid_t(xmlpp::Element* xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  void configure() { n_channels = 1; };
  double a;
};

cardioid_t::data_t::data_t(uint32_t chunksize)
{
  w = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

cardioid_t::cardioid_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),a(0)
{
  GET_ATTRIBUTE_(a);
}

void cardioid_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t psrc(prel.normal());
  float dw((1.0 + a*(psrc.x - 1.0) - d->w)*d->dt);
  for(uint32_t k=0;k<chunk.size();k++)
    output[0][k] += chunk[k]*(d->w+=dw);
}

void cardioid_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  float aw(sqrt(2.0)*(1.0-a));
  for(uint32_t k=0;k<chunk.size();k++)
    output[0][k] += aw*chunk.w()[k] + a*chunk.x()[k];
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
