#include "sourcemod.h"

class cardioidmod_t : public TASCAR::sourcemod_base_t {
public:
  class data_t : public TASCAR::sourcemod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    double dt;
    double w;
    double state;
  };
  cardioidmod_t(xmlpp::Element* xmlsrc);
  bool read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  TASCAR::sourcemod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  void configure() { n_channels = 1; };
  double f6db;
  double fmin;
private:
  double wpow;
  double wmin;
};

cardioidmod_t::data_t::data_t(uint32_t chunksize)
  : dt(1.0/std::max(1.0,(double)chunksize)),
    w(0),state(0)
{
}

cardioidmod_t::cardioidmod_t(xmlpp::Element* xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc),
    f6db(1000),
    fmin(60),
    wpow(1),
    wmin(EPS)
{
  GET_ATTRIBUTE(f6db,"Hz","Frequency in Hz, at which a 6~dB attenuation at 90 degrees is achieved");
  GET_ATTRIBUTE(fmin,"Hz","Low-end limit for stabilization");
}

bool cardioidmod_t::read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t prel_norm(prel.normal());
  // calculate panning parameters (as incremental values):
  double w(pow(0.5-0.5*prel_norm.x,wpow));
  if( w > wmin )
    w = wmin;
  if( !(w > EPS) )
    w = EPS;
  double dw((w - d->w)*d->dt);
  // apply panning:
  uint32_t N(output.size());
  for(uint32_t k=0;k<N;++k){
    output[k] = (d->state = input[0][k]*(1.0f-d->w) + d->state*d->w);
    d->w+=dw;
  }
  return false;
}

TASCAR::sourcemod_base_t::data_t* cardioidmod_t::create_data(double srate,uint32_t fragsize)
{
  wpow = log(exp(-M_PI*f6db/srate))/log(0.5);
  wmin = exp(-M_PI*fmin/srate);
  return new data_t(fragsize);
}

REGISTER_SOURCEMOD(cardioidmod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
