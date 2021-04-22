#include "receivermod.h"
#include "amb33defs.h"
#include "errorhandling.h"

class amb3h3v_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB33::idx::channels];
    float w_current[AMB33::idx::channels];
    float dw[AMB33::idx::channels];
    double dt;
  };
  amb3h3v_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure();
};


amb3h3v_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k=0;k<AMB33::idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

amb3h3v_t::amb3h3v_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
}

void amb3h3v_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  if( output.size() != AMB33::idx::channels ){
    DEBUG(output.size());
    DEBUG(AMB33::idx::channels);
    throw TASCAR::ErrMsg("Fatal error.");
  }
  data_t* d((data_t*)sd);
  float az = prel.azim();
  float el = prel.elev();
  float t, x2, y2, z2;
  // this is taken from AMB plugins by Fons and Joern:
  d->_w[AMB33::idx::w] = MIN3DB;
  t = cosf (el);
  d->_w[AMB33::idx::x] = t * cosf (az);
  d->_w[AMB33::idx::y] = t * sinf (az);
  d->_w[AMB33::idx::z] = sinf (el);
  x2 = d->_w[AMB33::idx::x] * d->_w[AMB33::idx::x];
  y2 = d->_w[AMB33::idx::y] * d->_w[AMB33::idx::y];
  z2 = d->_w[AMB33::idx::z] * d->_w[AMB33::idx::z];
  d->_w[AMB33::idx::u] = x2 - y2;
  d->_w[AMB33::idx::v] = 2 * d->_w[AMB33::idx::x] * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::s] = 2 * d->_w[AMB33::idx::z] * d->_w[AMB33::idx::x];
  d->_w[AMB33::idx::t] = 2 * d->_w[AMB33::idx::z] * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::r] = (3 * z2 - 1) / 2;
  d->_w[AMB33::idx::p] = (x2 - 3 * y2) * d->_w[AMB33::idx::x];
  d->_w[AMB33::idx::q] = (3 * x2 - y2) * d->_w[AMB33::idx::y];
  t = 2.598076f * d->_w[AMB33::idx::z]; 
  d->_w[AMB33::idx::n] = t * d->_w[AMB33::idx::u];
  d->_w[AMB33::idx::o] = t * d->_w[AMB33::idx::v];
  t = 0.726184f * (5 * z2 - 1);
  d->_w[AMB33::idx::l] = t * d->_w[AMB33::idx::x];
  d->_w[AMB33::idx::m] = t * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::k] = d->_w[AMB33::idx::z] * (5 * z2 - 3) / 2;
  for(unsigned int k=0;k<AMB33::idx::channels;k++)
    d->dw[k] = (d->_w[k] - d->w_current[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<AMB33::idx::channels;k++){
      output[k][i] += (d->w_current[k] += d->dw[k]) * chunk[i];
    }
  }
}

void amb3h3v_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  for( unsigned int i=0;i<chunk.size();i++){
    output[AMB33::idx::w][i] += chunk.w()[i];
    output[AMB33::idx::x][i] += chunk.x()[i];
    output[AMB33::idx::y][i] += chunk.y()[i];
    output[AMB33::idx::z][i] += chunk.z()[i];
  }
}

TASCAR::receivermod_base_t::data_t* amb3h3v_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(fragsize);
}

void amb3h3v_t::configure()
{
  n_channels = AMB33::idx::channels;
  labels.clear();
  for(uint32_t ch=0;ch<n_channels;++ch){
    char ctmp[32];
    sprintf(ctmp,".%g%c",floor(sqrt((double)ch)),AMB33::channelorder[ch]);
    labels.push_back( ctmp );
  }
}

REGISTER_RECEIVERMOD(amb3h3v_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
