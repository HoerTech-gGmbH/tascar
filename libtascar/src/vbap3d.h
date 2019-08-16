#ifndef VBAP3D_H
#define VBAP3D_H

#include "speakerarray.h"

namespace TASCAR {

  /**
     \brief 3D-VBAP encoding class
   */
  class vbap3d_t {
  public:
    /**
       \param spkarray Speaker array for which the encoder is generated
     */
    vbap3d_t(const std::vector<TASCAR::pos_t>& spkarray);
    virtual ~vbap3d_t();
    /**
       \brief Generate encoding weights for each speaker channel, based on unit vector of target direction

       \param prelnorm Unit vector of target direction

       Results are stored in the member vbap3d_t::weights
    */
    void encode( const TASCAR::pos_t& prelnorm );
    const uint32_t numchannels;
  private:
    float* channelweights;
  public:
    const float* weights;
  private:
    class simplex_t {
    public:
      simplex_t() : c1(-1),c2(-1),c3(-1){};
      bool get_gain( const TASCAR::pos_t& p){
        g1 = p.x*l11+p.y*l21+p.z*l31;
        g2 = p.x*l12+p.y*l22+p.z*l32;
        g3 = p.x*l13+p.y*l23+p.z*l33;
        if( (g1>=0.0) && (g2>=0.0) && (g3>=0.0)){
          double w(sqrt(g1*g1+g2*g2+g3*g3));
          if( w > 0 )
            w = 1.0/w;
          g1*=w;
          g2*=w;
          g3*=w;
          return true;
        }
        return false;
      };
      uint32_t c1;
      uint32_t c2;
      uint32_t c3;
      double l11;
      double l12;
      double l13;
      double l21;
      double l22;
      double l23;
      double l31;
      double l32;
      double l33;
      double g1;
      double g2;
      double g3;
    };
    std::vector<simplex_t> simplices;
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
