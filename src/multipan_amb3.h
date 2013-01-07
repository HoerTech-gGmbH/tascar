/**
   \file multipan_amb3.h
   \ingroup libtascar
   \brief "dynpan" is a multichannel panning tool implementing several panning methods.
   \author Giso Grimm
   \date 2012

   \section license License (GPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Lesser Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#ifndef MULTIPAN_AMB3_H
#define MULTIPAN_AMB3_H

#include "multipan.h"
#include "ambpan.h"
#include "hoafilt/hoafilt.h"

namespace TASCAR {

  class pan_amb3_basic_t : public pan_base_t {
  public:
    pan_amb3_basic_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    pos_t src;
    float w,x,y,u,v,p,q;
    HoS::amb_coeff amb_w;
    std::vector<double> spk_delay;
    std::vector<double> spk_gain;
    std::vector<std::vector<double> > decoder_matrix;
  };

  class pan_amb3_basic_nfc_t : public pan_base_t {
  public:
    pan_amb3_basic_nfc_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling, double fs);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    double fs_;
    pos_t src;
    float w,x,y,u,v,p,q;
    HoS::amb_coeff amb_w;
    std::vector<double> spk_delay;
    std::vector<double> spk_gain;
    std::vector<std::vector<double> > decoder_matrix;
    // this is very inefficient (calc each filter twice), just for fast hack:
    NF_filt1 f1a;
    NF_filt1 f1b;
    NF_filt2 f2a;
    NF_filt2 f2b;
    NF_filt3 f3a;
    NF_filt3 f3b;
  };

}

#endif


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
