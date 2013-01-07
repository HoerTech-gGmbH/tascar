#include "multipan_amb3.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include "defs.h"

using namespace TASCAR;

pan_amb3_basic_t::pan_amb3_basic_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling)
  : pan_base_t(spk,dline,subsampling),
    amb_w(subsampling)
{
  spk_delay.resize(spk_.n);
  spk_gain.resize(spk_.n);
  decoder_matrix.resize(spk_.n);
  for( unsigned int k=0;k<decoder_matrix.size();k++)
    decoder_matrix[k].resize(7);
  for(unsigned int k=0;k<spk_.n;k++){
    spk_delay[k] = spk_.d_max - spk_.d[k];
    spk_gain[k] = spk_.d[k]/spk_.d_max;
    // set decoder matrix:
    decoder_matrix[k][0] = 1.0/spk_.n;
    for(unsigned int kOrder=1;kOrder<4;kOrder++){
      decoder_matrix[k][2*kOrder-1] = 2*cos((double)kOrder*spk_.az[k]) / spk_.n;
      decoder_matrix[k][2*kOrder] = 2*sin((double)kOrder*spk_.az[k]) / spk_.n;
    }
  }
}

void pan_amb3_basic_t::updatepar()
{
  amb_w.set(src.azim(),0,1);
}

void pan_amb3_basic_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    double d = src.norm();
    amb_w.clear(w,x,y,u,v,p,q);
    amb_w.addpan30(delayline.get_dist( d )/std::max(1.0,d),w,x,y,u,v,p,q);
    for( unsigned int k=0;k<spk_.n;k++){
      outBuffer[k][i] += 
        decoder_matrix[k][0]*w + 
        decoder_matrix[k][1]*x +
        decoder_matrix[k][2]*y +
        decoder_matrix[k][3]*u + 
        decoder_matrix[k][4]*v +
        decoder_matrix[k][5]*p + 
        decoder_matrix[k][6]*q; 
    }
  }
}


pan_amb3_basic_nfc_t::pan_amb3_basic_nfc_t(speakerlayout_t& spk,varidelay_t& dline,uint32_t subsampling,double fs)
  : pan_base_t(spk,dline,subsampling),
    fs_(fs),
    amb_w(subsampling)
{
  spk_delay.resize(spk_.n);
  spk_gain.resize(spk_.n);
  decoder_matrix.resize(spk_.n);
  for( unsigned int k=0;k<decoder_matrix.size();k++)
    decoder_matrix[k].resize(7);
  for(unsigned int k=0;k<spk_.n;k++){
    spk_delay[k] = spk_.d_max - spk_.d[k];
    spk_gain[k] = spk_.d[k]/spk_.d_max;
    // set decoder matrix:
    decoder_matrix[k][0] = 1.0/spk_.n;
    for(unsigned int kOrder=1;kOrder<4;kOrder++){
      decoder_matrix[k][2*kOrder-1] = 2*cos((double)kOrder*spk_.az[k]) / spk_.n;
      decoder_matrix[k][2*kOrder] = 2*sin((double)kOrder*spk_.az[k]) / spk_.n;
    }
  }
  f1a.reset();
  f2a.reset();
  f3a.reset();
  f1b.reset();
  f2b.reset();
  f3b.reset();
}

void pan_amb3_basic_nfc_t::updatepar()
{
  amb_w.set(src.azim(),0,1);
  double d1 = 340.0/std::max(1.0,src.norm()*fs_);
  double d2 = 340.0/(spk_.d_max*fs_);
  f1a.init (d1, d2);
  f2a.init (d1, d2);
  f3a.init (d1, d2);
  f1b.init (d1, d2);
  f2b.init (d1, d2);
  f3b.init (d1, d2);
}

void pan_amb3_basic_nfc_t::process(uint32_t n, float* vIn, float* vX, float* vY, float* vZ, const std::vector<float*>& outBuffer)
{
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]);
    src.set_cart(vX[i],vY[i],vZ[i]);
    inct();
    double d = src.norm();
    amb_w.clear(w,x,y,u,v,p,q);
    amb_w.addpan30(delayline.get_dist( d )/std::max(1.0,d),w,x,y,u,v,p,q);
    // this is very inefficient, don't call filter twice, don't call
    // them for each sample:
    f1a.process(1,&x,&x);
    f1b.process(1,&y,&y);
    f2a.process(1,&u,&u);
    f2b.process(1,&v,&v);
    f3a.process(1,&p,&p);
    f3b.process(1,&q,&q);
    for( unsigned int k=0;k<spk_.n;k++){
      outBuffer[k][i] += 
        decoder_matrix[k][0]*w + 
        decoder_matrix[k][1]*x +
        decoder_matrix[k][2]*y +
        decoder_matrix[k][3]*u + 
        decoder_matrix[k][4]*v +
        decoder_matrix[k][5]*p + 
        decoder_matrix[k][6]*q; 
    }
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
