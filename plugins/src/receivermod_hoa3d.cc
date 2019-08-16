#include "receivermod.h"
#include "errorhandling.h"
#include "hoa.h"

class hoa3d_dec_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t order);
    // ambisonic weights:
    std::vector<float> B;
  };
  hoa3d_dec_t(xmlpp::Element* xmlsrc);
  ~hoa3d_dec_t();
  void prepare( chunk_cfg_t& );
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void postproc(std::vector<TASCAR::wave_t>& output);
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  int32_t order;
  std::string method;
  std::string dectype;
  uint32_t channels;
  HOA::encoder_t encode;
  HOA::decoder_t decode;
  std::vector<float> B;
  std::vector<float> deltaB;
  std::vector<TASCAR::wave_t> amb_sig;
};

void hoa3d_dec_t::prepare( chunk_cfg_t& cf_ )
{
  TASCAR::receivermod_base_speaker_t::prepare( cf_ );
  amb_sig = std::vector<TASCAR::wave_t>(channels,TASCAR::wave_t(n_fragment));
}

hoa3d_dec_t::data_t::data_t(uint32_t channels )
{
  B = std::vector<float>(channels, 0.0f );
}

hoa3d_dec_t::hoa3d_dec_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc),
  order(3),
  method("pinv"),
  dectype("maxre")
{
  GET_ATTRIBUTE(order);
  GET_ATTRIBUTE(method);
  GET_ATTRIBUTE(dectype);
  if( order < 0 )
    throw TASCAR::ErrMsg("Negative order is not possible.");
  encode.set_order( order );
  channels = (order+1)*(order+1);
  B = std::vector<float>(channels, 0.0f );
  deltaB = std::vector<float>(channels, 0.0f );
  if( method == "pinv" )
    decode.create_pinv( order, spkpos.get_positions() );
  else if( method == "allrad" )
    decode.create_allrad( order, spkpos.get_positions() );
  else
    throw TASCAR::ErrMsg("Invalid decoder generation method \""+method+"\".");
  if( dectype == "basic" )
    decode.modify( HOA::decoder_t::basic );
  else if( dectype == "maxre" )
    decode.modify( HOA::decoder_t::maxre );
  else if( dectype == "inphase" )
    decode.modify( HOA::decoder_t::inphase );
  else
    throw TASCAR::ErrMsg("Invalid decoder type \""+dectype+"\".");
}

hoa3d_dec_t::~hoa3d_dec_t()
{
}

void hoa3d_dec_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* state(dynamic_cast<data_t*>(sd));
  if( !state )
    throw TASCAR::ErrMsg("Invalid data type.");
  float az = prel.azim();
  float el = prel.elev();
  encode(az,el,B);
  for(uint32_t index=0;index<channels;++index)
    deltaB[index] = (B[index] - state->B[index])*t_inc;
  for(uint32_t t=0;t<chunk.size();++t)
    for(uint32_t index=0;index<channels;++index)
      amb_sig[index][t] += (state->B[index] += deltaB[index]) * chunk[t];
  for(uint32_t index=0;index<channels;++index)
    state->B[index] = B[index];
}

void hoa3d_dec_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  decode( amb_sig, output );
  for( uint32_t acn=0;acn<channels;++acn)
    amb_sig[acn].clear();
  TASCAR::receivermod_base_speaker_t::postproc(output);
}

TASCAR::receivermod_base_t::data_t* hoa3d_dec_t::create_data(double srate, uint32_t fragsize)
{
  return new data_t(channels);
}

REGISTER_RECEIVERMOD(hoa3d_dec_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
