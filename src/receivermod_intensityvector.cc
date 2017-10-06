#include "receivermod.h"

class intensityvector_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    virtual ~data_t();
    float lpstate;
    float x;
    float y;
    float z;
    double dt;
  };
  intensityvector_t(xmlpp::Element* xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  uint32_t get_num_channels();
  void prepare( chunk_cfg_t& );
  std::string get_channel_postfix(uint32_t channel) const;
private:
  double tau;
  double c1;
  double c2;
};

intensityvector_t::intensityvector_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    tau(0.125),
    c1(1),
    c2(0)
{
  GET_ATTRIBUTE(tau);
}

void intensityvector_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t psrc(prel.normal());
  float dx((psrc.x - d->x)*d->dt);
  float dy((psrc.y - d->y)*d->dt);
  float dz((psrc.z - d->z)*d->dt);
  for( unsigned int i=0;i<chunk.size();i++){
    float intensity(chunk[i]);
    intensity *= intensity;
    output[0][i] = (d->lpstate = c1*intensity + c2*d->lpstate);
    output[1][i] = (d->x += dx) * d->lpstate;
    output[2][i] = (d->y += dy) * d->lpstate;
    output[3][i] = (d->z += dz) * d->lpstate;
  }
}

void intensityvector_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
}

uint32_t intensityvector_t::get_num_channels()
{
  return 4;
}

intensityvector_t::data_t::data_t(uint32_t chunksize)
  : lpstate(0),
    x(0),
    y(0),
    z(0),
    dt(1.0/std::max(1.0,(double)chunksize))
{
}

intensityvector_t::data_t::~data_t()
{
}

void intensityvector_t::prepare( chunk_cfg_t& cf_ )
{
  TASCAR::receivermod_base_t::prepare( cf_ );
  c1 = exp(-1.0/(tau*f_sample));
  c2 = 1.0-c1;
}

TASCAR::receivermod_base_t::data_t* intensityvector_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize);
}


std::string intensityvector_t::get_channel_postfix(uint32_t channel) const
{
  switch( channel ){
  case 0 : return "_norm";
  case 1 : return "_x";
  case 2 : return "_y";
  case 3 : return "_z";
  }
  return "_other";
}

REGISTER_RECEIVERMOD(intensityvector_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
