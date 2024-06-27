/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "errorhandling.h"
#include "hoa.h"
#include "receivermod.h"
#include <fstream>

class hoa3d_dec_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t order);
    // ambisonic weights:
    std::vector<float> B;
  };
  hoa3d_dec_t(tsccfg::node_t xmlsrc);
  ~hoa3d_dec_t();
  void configure();
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void postproc(std::vector<TASCAR::wave_t>& output);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  int32_t order;
  std::string method;
  std::string dectype;
  bool savedec;
  uint32_t channels;
  HOA::encoder_t encode;
  HOA::decoder_t decode;
  std::vector<float> B;
  std::vector<float> deltaB;
  std::vector<TASCAR::wave_t> amb_sig;
  double decwarnthreshold;
  bool allowallrad = false;
};

void hoa3d_dec_t::configure()
{
  TASCAR::receivermod_base_speaker_t::configure();
  amb_sig = std::vector<TASCAR::wave_t>(channels, TASCAR::wave_t(n_fragment));
}

hoa3d_dec_t::data_t::data_t(uint32_t channels)
{
  B = std::vector<float>(channels, 0.0f);
}

hoa3d_dec_t::hoa3d_dec_t(tsccfg::node_t xmlsrc)
    : TASCAR::receivermod_base_speaker_t(xmlsrc), order(3), method("pinv"),
      dectype("maxre"), savedec(false), decwarnthreshold(8.0)
{
  GET_ATTRIBUTE_BOOL(allowallrad, "All to use AllRAD decoder despite current "
                                  "inconsistency across OS and compiler");
  GET_ATTRIBUTE(order, "", "Ambisonics order");
  GET_ATTRIBUTE(method, "",
                "Decoder generation method, ``pinv'' or ``allrad''");
  GET_ATTRIBUTE(dectype, "",
                "Decoder type, ``basic'', ``maxre'' or ``inphase''");
  GET_ATTRIBUTE_BOOL(savedec,
                     "Save Octave/Matlab script for decoder matrix debugging");
  GET_ATTRIBUTE(decwarnthreshold, "",
                "Warning threshold for decoder matrix abs/rms ratio");
  if(order < 0)
    throw TASCAR::ErrMsg("Negative order is not possible.");
  if(!allowallrad && (method == "allrad"))
    throw TASCAR::ErrMsg(
        "The AllRAD method creates inconsistent results on different operating "
        "systems and is therefore "
        "currently not recommended to use. If you really want to use it, set "
        "\"allowallrad\" to \"true\".");
  encode.set_order(order);
  channels = (order + 1) * (order + 1);
  B = std::vector<float>(channels, 0.0f);
  deltaB = std::vector<float>(channels, 0.0f);
  if(method == "pinv")
    decode.create_pinv(order, spkpos.get_positions());
  else if(method == "allrad")
    decode.create_allrad(order, spkpos.get_positions());
  else
    throw TASCAR::ErrMsg("Invalid decoder generation method \"" + method +
                         "\".");
  if(dectype == "basic")
    decode.modify(HOA::decoder_t::basic);
  else if(dectype == "maxre")
    decode.modify(HOA::decoder_t::maxre);
  else if(dectype == "inphase")
    decode.modify(HOA::decoder_t::inphase);
  else
    throw TASCAR::ErrMsg("Invalid decoder type \"" + dectype + "\".");
  typeidattr.push_back("order");
  typeidattr.push_back("method");
  typeidattr.push_back("dectype");
  float r;
  if((r = decode.maxabs() / decode.rms()) > decwarnthreshold) {
    TASCAR::add_warning("The maximum-to-rms ratio of the decoder matrix is " +
                        TASCAR::to_string(r) +
                        ".\nThis might mean that the matrix is not optimal.");
  }
  if(savedec) {
    std::ofstream ofh(
        "decoder_" +
        TASCAR::strrep(TASCAR::strrep(get_spktypeid(), ":", ""), ",", "_") +
        ".m");
    ofh << decode.to_string() << std::endl;
  }
}

hoa3d_dec_t::~hoa3d_dec_t() {}

void hoa3d_dec_t::add_pointsource(const TASCAR::pos_t& prel, double,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>&,
                                  receivermod_base_t::data_t* sd)
{
  data_t* state(dynamic_cast<data_t*>(sd));
  if(!state)
    throw TASCAR::ErrMsg("Invalid data type.");
  float az = prel.azim();
  float el = prel.elev();
  encode(az, el, B);
  for(uint32_t index = 0; index < channels; ++index)
    deltaB[index] = (B[index] - state->B[index]) * t_inc;
  for(uint32_t t = 0; t < chunk.size(); ++t)
    for(uint32_t index = 0; index < channels; ++index)
      amb_sig[index][t] += (state->B[index] += deltaB[index]) * chunk[t];
  for(uint32_t index = 0; index < channels; ++index)
    state->B[index] = B[index];
}

void hoa3d_dec_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  decode(amb_sig, output);
  for(uint32_t acn = 0; acn < channels; ++acn)
    amb_sig[acn].clear();
  TASCAR::receivermod_base_speaker_t::postproc(output);
}

TASCAR::receivermod_base_t::data_t*
hoa3d_dec_t::create_state_data(double, uint32_t) const
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
