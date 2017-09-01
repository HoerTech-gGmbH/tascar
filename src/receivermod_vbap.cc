#include "errorhandling.h"
#include "scene.h"
#include <complex.h>

class rec_vbap_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels);//construct
    virtual ~data_t();
    // ambisonic weights:
    float* wp;
    float* dwp;
    float* w;
    float* dw;
    float* x;
    float* dx;
    float* y;
    float* dy;
    float* z;
    float* dz;
    double dt;
  };
  rec_vbap_t(xmlpp::Element* xmlsrc);
  virtual ~rec_vbap_t() {};
  void write_xml(); //fun.dec.
  //declaration add_pointsource
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);//declaration add_pointsource
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  //TASCAR::Scene::spk_array_t spkpos;
};

//function definition:  write_xml
void rec_vbap_t::write_xml()
{
  //SET_ATTRIBUTE(wexp);
}

//constructor definition for data_t
rec_vbap_t::data_t::data_t(uint32_t chunksize,uint32_t channels)
{
  wp = new float[channels];
  dwp = new float[channels];
  w = new float[channels];
  dw = new float[channels];
  x = new float[channels];
  dx = new float[channels];
  y = new float[channels];
  dy = new float[channels];
  z = new float[channels];
  dz = new float[channels];
  for(uint32_t k=0;k<channels;k++)
    wp[k] = dwp[k] = w[k] = dw[k] = x[k] = dx[k] = y[k] = dy[k] = z[k] = dz[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

//definition ~data_t()
rec_vbap_t::data_t::~data_t()
{
  delete [] wp;
  delete [] dwp;
  delete [] w;
  delete [] dw;
  delete [] x;
  delete [] dx;
  delete [] y;
  delete [] dy;
  delete [] z;
  delete [] dz;
}

// constructor; mainly parsing the XML file
rec_vbap_t::rec_vbap_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc)
{
  if( spkpos.size() < 2 )
    throw TASCAR::ErrMsg("At least two loudspeakers are required for 2D-VBAP.");
}

//definition of the function add_pointsource
void rec_vbap_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)

/*INPUT:
  prel   : position of the source in the receiver coordinate system (from the receiver perspective)
  output :a vector with type wave_t and wave_t can be found in audiochunks.h and is a class of time domain audio chunks (?)
  so for each loudspeaker theres is one audiochunk (so sth like a portion of audio data..?)
  chunk  :a signle element of type wave_t
  sd     :variable of type data_t  so it contains all these wp, dwp, w,... */

{
  // N is the number of loudspeakers:
  uint32_t N(output.size());

  // d is the internal state variable for this specific
  // receiver-source-pair:
  data_t* d((data_t*)sd);//it creates the variable d

  // psrc_normal is the normalized source direction in the receiver
  // coordinate system:
  TASCAR::pos_t psrc_normal(prel.normal());

  //**********************************  2D  VBAP  *************************************

  //-------------1.Search two nearest speakers:--------------
  const std::vector<TASCAR::spk_array_t::didx_t>& didx(spkpos.sort_distance(psrc_normal));
  
  // 1.a) Find the first closest loudspeaker 
  uint32_t kmin1(didx[0].idx);
  // 1.b) Find the second closest loudspeaker 
  uint32_t kmin2(didx[1].idx);
  // now kmin1 and kmin2 are nearest speakers numbers and dmin1 and dmin2 smallest distances.


  //-------------2.Calculate the inverse square matrix:--------------

  double l11(spkpos[kmin1].unitvector.x);
  double l12(spkpos[kmin1].unitvector.y);
  double l21(spkpos[kmin2].unitvector.x);
  double l22(spkpos[kmin2].unitvector.y);
  double det_speaker(l11*l22 - l21*l12);
  if( det_speaker != 0 )
    det_speaker = 1.0/det_speaker;
  double linv11(det_speaker*l22);
  double linv12(-det_speaker*l12);
  double linv21(-det_speaker*l21);
  double linv22(det_speaker*l11);
  // calculate speaker weights:
  double g1(psrc_normal.x*linv11+psrc_normal.y*linv21);
  double g2(psrc_normal.x*linv12+psrc_normal.y*linv22);
  double w(sqrt(g1*g1+g2*g2));
  if( w > 0 )
    w = 1.0/w;
  double w_kmin1(w*g1);
  double w_kmin2(w*g2);
 
//
//  //double L_11 =spk.normal[kmin1].x;
//  //double L_21 =spk.normal[kmin1].y;
//  //double L_12 =spk.normal[kmin2].x;
//  //double L_22 =spk.normal[kmin2].y;
//
//  double det_inv = 1.0/(spkpos[kmin1].unitvector.x*spkpos[kmin2].unitvector.y - 
//                        spkpos[kmin2].unitvector.x*spkpos[kmin1].unitvector.y);
//
//  double L_inv_11 =det_inv*spkpos[kmin2].unitvector.y;
//  double L_inv_21 =det_inv*-spkpos[kmin1].unitvector.y;
//  double L_inv_12 =det_inv*-spkpos[kmin2].unitvector.x;
//  double L_inv_22 =det_inv*spkpos[kmin1].unitvector.x;
//
//  //-------------3.Calculate the gains for louspeakers according to formula [w1 w2]=[p1 p2]*L^-1  :--------------
//
//  double w_kmin1= psrc_normal.x*L_inv_11+ psrc_normal.y*L_inv_21;
//  double w_kmin2= psrc_normal.x*L_inv_12+ psrc_normal.y*L_inv_22;
// 
//

  //-------------4.Apply the gain into the signal  :--------------

  //We get information about the gain (w) for loudspeaker only once in each block so we know the value of the gain at the beginning  and at the end of each block (at the beginning - value from the previous frame, at the end - value calculated in this iteration). Because we dont know the exact value for each time sample, we have to linearly interpolate between the two values that we have. To do this we have to compute the increment, and then this increment is added at every sample. 

  //4.a) Compute increment (dwp) based on current value (w_kmin) and previous value (wp) and number of samples dt =1/NrSamples
  for(unsigned int k=0;k<N;k++){
    d->dwp[k] = (0 - d->wp[k])*d->dt;//dwp=(0-wp)*dt
  }

  d->dwp[kmin1] = (w_kmin1 - d->wp[kmin1])*d->dt;// (dwp=G-wp)*dt 
  d->dwp[kmin2] = (w_kmin2 - d->wp[kmin2])*d->dt;
 
  //4.b) Loop for each channel at each time sample we compute the new value which is equall to the last value plus the increment 

  // i is time (in samples):
  for( unsigned int i=0;i<chunk.size();i++){
    // k is output channel number:
    for( unsigned int k=0;k<N;k++){
      //output in each louspeaker k at sample i: 
      //d->wp[k] += d->dwp[k];
      //output[k][i] = d->wp[k] * chunk[i];
      output[k][i] += (d->wp[k] += d->dwp[k]) * chunk[i]; // This += is because we sum up all the sources for which we call this func
    }
  }
}

void rec_vbap_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  spkpos.foa_decode(chunk,output);
}

uint32_t rec_vbap_t::get_num_channels()
{
  return spkpos.size();
}

std::string rec_vbap_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  sprintf(ctmp,".%d%s",channel,spkpos[channel].label.c_str());
  return ctmp;
}


TASCAR::receivermod_base_t::data_t* rec_vbap_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize,spkpos.size());
}

REGISTER_RECEIVERMOD(rec_vbap_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
