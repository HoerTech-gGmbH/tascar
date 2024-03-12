/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "amb33defs.h"
#include "errorhandling.h"
#include "receivermod.h"

#include "fft.h"
#include <limits>

class foaconv_vars_t : public TASCAR::receivermod_base_t {
public:
  foaconv_vars_t(tsccfg::node_t xmlsrc);
  ~foaconv_vars_t();

protected:
  std::string irsname;
  uint32_t maxlen;
  uint32_t offset;
};

foaconv_vars_t::foaconv_vars_t(tsccfg::node_t xmlsrc)
    : receivermod_base_t(xmlsrc), maxlen(0), offset(0)
{
  GET_ATTRIBUTE(irsname, "", "Name of IRS sound file");
  GET_ATTRIBUTE(maxlen, "samples",
                "Maximum length of IRS, or 0 to use full sound file");
  GET_ATTRIBUTE(offset, "samples", "Offset of IR in sound file");
}

foaconv_vars_t::~foaconv_vars_t() {}

class foaconv_t : public foaconv_vars_t {
public:
  foaconv_t(tsccfg::node_t xmlsrc);
  ~foaconv_t();
  void postproc(std::vector<TASCAR::wave_t>& output);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t&,
                               std::vector<TASCAR::wave_t>&,
                               receivermod_base_t::data_t*){};
  void configure();
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;

private:
  TASCAR::sndfile_t irs1;
  TASCAR::sndfile_t irs2;
  TASCAR::sndfile_t irs3;
  TASCAR::sndfile_t irs4;
  std::vector<TASCAR::partitioned_conv_t*> conv;
  TASCAR::wave_t* rec_out;
  float wgain;
  bool is_acn;
};

foaconv_t::foaconv_t(tsccfg::node_t cfg)
    : foaconv_vars_t(cfg), irs1(irsname, 0), irs2(irsname, 1), irs3(irsname, 2),
      irs4(irsname, 3), rec_out(NULL), wgain(MIN3DB), is_acn(false)
{
  std::string normalization("FuMa");
  GET_ATTRIBUTE(normalization, "",
                "Normalization of FOA response, either ``FuMa'' or ``SN3D''");
  if(normalization == "FuMa")
    wgain = MIN3DB;
  else if(normalization == "SN3D")
    wgain = 1.0f;
  else
    throw TASCAR::ErrMsg(
        "Currently, only FuMa and SN3D normalization is supported.");
  irs1 *= wgain;
  std::string channelorder("ACN");
  GET_ATTRIBUTE(channelorder, "",
                "Channel order of FOA response, either ``FuMa'' (wxyz) or "
                "``ACN'' (wyzx)");
  if(channelorder == "FuMa")
    is_acn = false;
  else if(channelorder == "ACN")
    is_acn = true;
  else
    throw TASCAR::ErrMsg(
        "Currently, only FuMa and ACN channelorder is supported.");
  if(offset >= irs1.get_frames())
    throw TASCAR::ErrMsg(
        "the offset (" + std::to_string(offset) +
        ") is larger than the length of the impulse response file (" +
        std::to_string(irs1.get_frames()) + ").");
}

void foaconv_t::configure()
{
  receivermod_base_t::configure();
  n_channels = AMB11ACN::idx::channels;
  for(auto it = conv.begin(); it != conv.end(); ++it)
    delete *it;
  conv.clear();
  uint32_t irslen(irs1.get_frames() - offset);
  if(maxlen > 0)
    irslen = std::min(maxlen, irslen);
  conv.push_back(new TASCAR::partitioned_conv_t(irslen, n_fragment));
  conv.push_back(new TASCAR::partitioned_conv_t(irslen, n_fragment));
  conv.push_back(new TASCAR::partitioned_conv_t(irslen, n_fragment));
  conv.push_back(new TASCAR::partitioned_conv_t(irslen, n_fragment));
  if(is_acn) {
    // wyzx
    conv[0]->set_irs(irs1, offset);
    conv[1]->set_irs(irs2, offset);
    conv[2]->set_irs(irs3, offset);
    conv[3]->set_irs(irs4, offset);
  } else {
    // wxyz
    conv[0]->set_irs(irs1, offset);
    conv[1]->set_irs(irs3, offset);
    conv[2]->set_irs(irs4, offset);
    conv[3]->set_irs(irs2, offset);
  }
  if(rec_out)
    delete rec_out;
  rec_out = new TASCAR::wave_t(n_fragment);
  labels.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    char ctmp[32];
    ctmp[31] = 0;
    snprintf(ctmp, 31, ".%d%c", (ch > 0), AMB11ACN::channelorder[ch]);
    labels.push_back(ctmp);
  }
}

foaconv_t::~foaconv_t()
{
  for(auto it = conv.begin(); it != conv.end(); ++it)
    delete *it;
  if(rec_out)
    delete rec_out;
}

void foaconv_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  for(uint32_t c = 0; c < 4; ++c)
    conv[c]->process(*rec_out, output[c], true);
  rec_out->clear();
}

TASCAR::receivermod_base_t::data_t* foaconv_t::create_state_data(double,
                                                                 uint32_t) const
{
  return new data_t();
}

void foaconv_t::add_pointsource(const TASCAR::pos_t&, double,
                                const TASCAR::wave_t& chunk,
                                std::vector<TASCAR::wave_t>&,
                                receivermod_base_t::data_t*)
{
  *rec_out += chunk;
}

REGISTER_RECEIVERMOD(foaconv_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
