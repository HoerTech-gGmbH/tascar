#include "receivermod.h"
#include "amb33defs.h"
#include "errorhandling.h"

class amb1h1v_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB11::idx::channels];
    float w_current[AMB11::idx::channels];
    float dw[AMB11::idx::channels];
    double dt;
  };
  amb1h1v_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  void configure();
  float wgain;
  float wgaindiff;
};


amb1h1v_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k=0;k<AMB11::idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

amb1h1v_t::amb1h1v_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
  std::string normalization("FuMa");
  GET_ATTRIBUTE(normalization,"","Normalization, either ``FuMa'' or ``SN3D''");
  if( normalization == "FuMa" )
    wgain = MIN3DB;
  else if( normalization == "SN3D" )
    wgain = 1.0f;
  else throw TASCAR::ErrMsg("Currently, only FuMa and SN3D normalization is supported.");
  wgaindiff = wgain/MIN3DB;
}

void amb1h1v_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  if( output.size() != AMB11::idx::channels ){
    DEBUG(output.size());
    DEBUG(AMB11::idx::channels);
    throw TASCAR::ErrMsg("Fatal error.");
  }
  TASCAR::pos_t pnorm(prel.normal());
  data_t* d((data_t*)sd);
  //float az = prel.azim();
  // this is more or less taken from AMB plugins by Fons and Joern:
  d->_w[AMB11::idx::w] = wgain;
  d->_w[AMB11::idx::x] = pnorm.x;
  d->_w[AMB11::idx::y] = pnorm.y;
  d->_w[AMB11::idx::z] = pnorm.z;
  for(unsigned int k=0;k<AMB11::idx::channels;k++)
    d->dw[k] = (d->_w[k] - d->w_current[k])*d->dt;
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<AMB11::idx::channels;k++){
      output[k][i] += (d->w_current[k] += d->dw[k]) * chunk[i];
    }
  }
}

void amb1h1v_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  for( unsigned int i=0;i<chunk.size();i++){
    output[AMB11::idx::w][i] += wgaindiff*chunk.w()[i];
    output[AMB11::idx::x][i] += chunk.x()[i];
    output[AMB11::idx::y][i] += chunk.y()[i];
    output[AMB11::idx::z][i] += chunk.z()[i];
  }
}

TASCAR::receivermod_base_t::data_t* amb1h1v_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize);
}

void amb1h1v_t::configure()
{
  receivermod_base_t::configure();
  n_channels = AMB11::idx::channels;
  labels.clear();
  for(uint32_t ch=0;ch<n_channels;++ch){
    char ctmp[32];
    sprintf(ctmp,".%d%c",(ch>0),AMB11::channelorder[ch]);
    labels.push_back(ctmp);
  }
}

REGISTER_RECEIVERMOD(amb1h1v_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
