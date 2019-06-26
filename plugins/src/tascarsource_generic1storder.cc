#include "sourcemod.h"

class generic1storder_t : public TASCAR::sourcemod_base_t {
public:
  class data_t : public TASCAR::sourcemod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    double dt;
    double w;
    double state;
  };
  generic1storder_t(xmlpp::Element* xmlsrc);
  bool read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  uint32_t get_num_channels();
  TASCAR::sourcemod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  double a;
};

generic1storder_t::data_t::data_t(uint32_t chunksize)
  : dt(1.0/std::max(1.0,(double)chunksize)),
    w(0),state(0)
{
}

generic1storder_t::generic1storder_t(xmlpp::Element* xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc),
  a(0)
{
  GET_ATTRIBUTE(a);
}

bool generic1storder_t::read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t psrc(prel.normal());
  float dw((1.0 + a*(psrc.x - 1.0) - d->w)*d->dt);
  uint32_t N(output.size());
  float* p_out(output.d);
  float* p_in(input[0].d);
  for(uint32_t k=0;k<N;++k){
    *p_out = *p_in * (d->w+=dw);
    ++p_out;
    ++p_in;
  }
  return false;
}

uint32_t generic1storder_t::get_num_channels()
{
  return 1;
}

TASCAR::sourcemod_base_t::data_t* generic1storder_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize);
}

REGISTER_SOURCEMOD(generic1storder_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
