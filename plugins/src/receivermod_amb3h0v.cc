#include "receivermod.h"
#include "amb33defs.h"
#include "errorhandling.h"

class amb3h0v_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB30::idx::channels];
    float w_current[AMB30::idx::channels];
    float dw[AMB30::idx::channels];
    double dt;
  };
  amb3h0v_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void configure();
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
};


amb3h0v_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k=0;k<AMB30::idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

amb3h0v_t::amb3h0v_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
}

void amb3h0v_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  if( output.size() != AMB30::idx::channels ){
    DEBUG(output.size());
    DEBUG(AMB30::idx::channels);
    throw TASCAR::ErrMsg("Fatal error.");
  }
  data_t* d((data_t*)sd);
  float az = prel.azim();
  float x2, y2;
  // this is more or less taken from AMB plugins by Fons and Joern:
  d->_w[AMB30::idx::w] = MIN3DB;
  d->_w[AMB30::idx::x] = cosf (az);
  d->_w[AMB30::idx::y] = sinf (az);
  x2 = d->_w[AMB30::idx::x] * d->_w[AMB30::idx::x];
  y2 = d->_w[AMB30::idx::y] * d->_w[AMB30::idx::y];
  d->_w[AMB30::idx::u] = x2 - y2;
  d->_w[AMB30::idx::v] = 2.0f * d->_w[AMB30::idx::x] * d->_w[AMB30::idx::y];
  d->_w[AMB30::idx::p] = (x2 - 3.0f * y2) * d->_w[AMB30::idx::x];
  d->_w[AMB30::idx::q] = (3.0f * x2 - y2) * d->_w[AMB30::idx::y];
  for(unsigned int k=0;k<AMB30::idx::channels;k++)
    d->dw[k] = (d->_w[k] - d->w_current[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<AMB30::idx::channels;k++){
      output[k][i] += (d->w_current[k] += d->dw[k]) * chunk[i];
    }
  }
}

void amb3h0v_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  for( unsigned int i=0;i<chunk.size();i++){
    output[AMB30::idx::w][i] += chunk.w()[i];
    output[AMB30::idx::x][i] += chunk.x()[i];
    output[AMB30::idx::y][i] += chunk.y()[i];
  }
}

void amb3h0v_t::configure()
{
  n_channels = AMB30::idx::channels;
  labels.clear();
  for(uint32_t ch=0;ch<n_channels;++ch){
    char ctmp[32];
    sprintf(ctmp,".%g%c",floor((double)(ch+1)*0.5),AMB30::channelorder[ch]);
    labels.push_back(ctmp);
  }
}

TASCAR::receivermod_base_t::data_t* amb3h0v_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize);
}

REGISTER_RECEIVERMOD(amb3h0v_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
