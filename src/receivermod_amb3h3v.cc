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
    float rotz_current[2];
    float drotz[2];
    double dt;
  };
  amb3h3v_t(xmlpp::Element* xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  std::string get_channel_postfix(uint32_t channel) const;
};


amb3h3v_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k=0;k<AMB33::idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  rotz_current[0] = 0;
  rotz_current[1] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

amb3h3v_t::amb3h3v_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
}

void amb3h3v_t::add_pointsource(const TASCAR::pos_t& prel, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  if( output.size() != AMB33::idx::channels ){
    DEBUG(output.size());
    DEBUG(AMB33::idx::channels);
    throw TASCAR::ErrMsg("Fatal error.");
  }
  data_t* d((data_t*)sd);
  float az = prel.azim();
  float x2, y2;
  // this is more or less taken from AMB plugins by Fons and Joern:
  d->_w[AMB33::idx::w] = MIN3DB;
  d->_w[AMB33::idx::x] = cosf (az);
  d->_w[AMB33::idx::y] = sinf (az);
  x2 = d->_w[AMB33::idx::x] * d->_w[AMB33::idx::x];
  y2 = d->_w[AMB33::idx::y] * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::u] = x2 - y2;
  d->_w[AMB33::idx::v] = 2.0f * d->_w[AMB33::idx::x] * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::p] = (x2 - 3.0f * y2) * d->_w[AMB33::idx::x];
  d->_w[AMB33::idx::q] = (3.0f * x2 - y2) * d->_w[AMB33::idx::y];
  for(unsigned int k=0;k<AMB33::idx::channels;k++)
    d->dw[k] = (d->_w[k] - d->w_current[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<AMB33::idx::channels;k++){
      output[k][i] += (d->w_current[k] += d->dw[k]) * chunk[i];
    }
  }
}

void amb3h3v_t::add_diffusesource(const TASCAR::pos_t& prel, const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float az = prel.azim();
  d->drotz[0] = (cos(az) - d->rotz_current[0])*d->dt;;
  d->drotz[1] = (sin(az) - d->rotz_current[1])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    d->rotz_current[0] += d->drotz[0];
    d->rotz_current[1] += d->drotz[1];
    float x(d->rotz_current[0]*chunk.x()[i] + d->rotz_current[1]*chunk.y()[i]);
    float y(-d->rotz_current[1]*chunk.x()[i] + d->rotz_current[0]*chunk.y()[i]);
    output[AMB33::idx::w][i] += chunk.w()[i];
    output[AMB33::idx::x][i] += x;
    output[AMB33::idx::y][i] += y;
  }
}

uint32_t amb3h3v_t::get_num_channels()
{
  return AMB33::idx::channels;
}

TASCAR::receivermod_base_t::data_t* amb3h3v_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize);
}

std::string amb3h3v_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[32];
  sprintf(ctmp,".%g%c",floor((double)(channel+1)*0.5),AMB33::channelorder[channel]);
  return ctmp;
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
