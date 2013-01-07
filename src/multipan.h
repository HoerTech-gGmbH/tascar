/**
   \file multipan.h
   \ingroup libtascar
   \brief "dynpan" is a multichannel panning tool implementing several panning methods.
   \author Giso Grimm
   \date 2012

   \section license License (LGPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Lesser Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#ifndef MULTIPAN_H
#define MULTIPAN_H

#include "delayline.h"
#include "speakerlayout.h"

/**
   \brief Spatial audio classes
   \ingroup libtascar
*/
namespace TASCAR {

  /**
     \ingroup libtascar
     \brief Base class for panning methods
   */
  class pan_base_t {
  public:
    pan_base_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling) : spk_(spk), delayline(dline), subsampling_(subsampling),dt1(1.0/(double)subsampling_),subsamplingpos(0){};
  protected:
    speakerlayout_t& spk_;
    varidelay_t& delayline;
    uint32_t subsampling_;
    double dt1;
    uint32_t subsamplingpos;
    inline void inct(void) {
      if( !subsamplingpos ){
        subsamplingpos = subsampling_;
        updatepar();
      }
      subsamplingpos--;
    };
  public:
    virtual void process(uint32_t n,float* vIn, float* vX, float* vY, float* vZ,const std::vector<float*>& outBuffer) = 0;
  protected:
    virtual void updatepar() = 0;
  };

  /**
     \ingroup libtascar
     \brief Simplest panning method: select nearest speaker
  */
  class pan_nearest_t : public pan_base_t {
  public:
    pan_nearest_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    pos_t src;
    uint32_t k_nearest;
  };

  /**
     \ingroup libtascar
     \brief Vector based amplitude panning
  */
  class pan_vbap_t : public pan_base_t {
  public:
    pan_vbap_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    pos_t src;
    uint32_t k_nearest;
    uint32_t k_next;
    double w_nearest;
    double w_next;
    double d_w_nearest;
    double d_w_next;
    uint32_t prev_k_nearest;
    uint32_t prev_k_next;
    double prev_w_nearest;
    double prev_w_next;
    double d_prev_w_nearest;
    double d_prev_w_next;
  };

  /**
     \ingroup libtascar
     \brief "basic" horizontal ambisonic panning method
      
     after Martin Neukom: Ambisonic Panning. AES Convention Paper
     7297, 2007
   */
  class pan_amb_basic_t : public pan_base_t {
  public:
    pan_amb_basic_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling,uint32_t max_order = 8192);
    void set_order(double order);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    pos_t src;
    std::vector<double> spk_delay;
    std::vector<double> spk_gain;
    std::vector<double> w_current;
    std::vector<double> dw;
    double scale_order;
  };

  /**
     \ingroup libtascar
     \brief "inphase" horizontal ambisonics panning method
     
     Method after Martin Neukom: Ambisonic
     Panning. AES Convention Paper 7297, 2007
   */
  class pan_amb_inphase_t : public pan_base_t {
  public:
    pan_amb_inphase_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling,uint32_t max_order = 8192);
    void set_order(double order);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    pos_t src;
    std::vector<double> spk_delay;
    std::vector<double> spk_gain;
    std::vector<double> w_current;
    std::vector<double> dw;
    double scale_order;
  };

  /**
     \ingroup libtascar
     \brief Wave field synthesis panning methods.
  */
  class pan_wfs_t : public pan_base_t {
  public:
    pan_wfs_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    pos_t src;
    std::vector<double> spk_dist;
    std::vector<double> d_spk_dist;
    std::vector<double> w;
    std::vector<double> d_w;
    double wfs_gain;
  };

  /**
     \ingroup libtascar
     \brief Cardioid microphone array simulation

     A set of cardioid microphones is placed at the points given in
     'speakerlayout'. The direction is always pointing outwards from
     the origin.
  */
  class pan_cardioid_t : public pan_base_t {
  public:
    pan_cardioid_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling);
  private:
    void updatepar();
    void process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer);
    pos_t src;
    std::vector<double> spk_dist;
    std::vector<double> d_spk_dist;
    std::vector<double> w;
    std::vector<double> dw;
  };

};

#endif


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
