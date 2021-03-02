#include "receivermod.h"
#include "hoa.h"

class hoa3d_enc_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t order);
    // ambisonic weights:
    std::vector<float> B;
  };
  hoa3d_enc_t(xmlpp::Element* xmlsrc);
  ~hoa3d_enc_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void configure() { n_channels = channels; };
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  int32_t order;
  uint32_t channels;
  HOA::encoder_t encode;
  std::vector<float> B;
  std::vector<float> deltaB;
};

hoa3d_enc_t::data_t::data_t(uint32_t channels )
{
  B = std::vector<float>(channels, 0.0f );
}

hoa3d_enc_t::hoa3d_enc_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
  order(3)
{
  GET_ATTRIBUTE_(order);
  if( order < 0 )
    throw TASCAR::ErrMsg("Negative order is not possible.");
  channels = (order+1)*(order+1);
  encode.set_order( order );
  B = std::vector<float>(channels, 0.0f );
  deltaB = std::vector<float>(channels, 0.0f );
}

hoa3d_enc_t::~hoa3d_enc_t()
{
}

void hoa3d_enc_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* state(dynamic_cast<data_t*>(sd));
  if( !state )
    throw TASCAR::ErrMsg("Invalid data type.");
  float az(prel.azim());
  float el(prel.elev());
  // calculate encoding weights:
  encode(az,el,B);
  // calculate incremental weights:
  for(uint32_t acn=0;acn<channels;++acn)
    deltaB[acn] = (B[acn] - state->B[acn])*t_inc;
  // apply weights:
  for(uint32_t t=0;t<chunk.size();++t)
    for(uint32_t acn=0;acn<channels;++acn)
      output[acn][t] += (state->B[acn] += deltaB[acn]) * chunk[t];
  // copy final values to avoid rounding errors:
  for(uint32_t acn=0;acn<channels;++acn)
    state->B[acn] = B[acn];
}

void hoa3d_enc_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  if( output.size() ){
    output[0].add( chunk.w(), sqrtf(2.0f) );
    if( output.size() > 3 ){
      output[1].add( chunk.y() );
      output[2].add( chunk.z() );
      output[3].add( chunk.x() );
    }
  }
}

TASCAR::receivermod_base_t::data_t* hoa3d_enc_t::create_data(double srate, uint32_t fragsize)
{
  return new data_t(channels);
}

REGISTER_RECEIVERMOD(hoa3d_enc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
