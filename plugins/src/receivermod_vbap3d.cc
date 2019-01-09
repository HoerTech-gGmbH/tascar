#include "errorhandling.h"
#include "scene.h"
#include <complex.h>

class rec_vbap3d_t : public TASCAR::receivermod_base_speaker_t {
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
  rec_vbap3d_t(xmlpp::Element* xmlsrc);
  virtual ~rec_vbap3d_t() {};
  //declaration add_pointsource
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);//declaration add_pointsource
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  //TASCAR::Scene::spk_array_t spkpos;
  //std::vector<double> spk_az;
  //std::vector<double> spk_gain;
  //std::vector<TASCAR::pos_t> spk_normal;
};

//constructor definition for data_t
rec_vbap3d_t::data_t::data_t(uint32_t chunksize,uint32_t channels)
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
rec_vbap3d_t::data_t::~data_t()
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

//???
rec_vbap3d_t::rec_vbap3d_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc)
{
  if( spkpos.size() < 3 )
    throw TASCAR::ErrMsg("At least three loudspeakers are required for 3D-VBAP.");
}



//definition of the function add_pointsource
void rec_vbap3d_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)

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

  //**********************************  3D  VBAP  *************************************

  //-------------1.Search three nearest speakers:--------------
  const std::vector<TASCAR::spk_array_t::didx_t>& didx(spkpos.sort_distance(psrc_normal));
  
  // 1.a) Find the first closest loudspeaker 
  uint32_t kmin1(didx[0].idx);
  // 1.b) Find the second closest loudspeaker 
  uint32_t kmin2(didx[1].idx);
  // 1.c) Find the third closest loudspeaker 
  uint32_t kmin3(didx[2].idx);

  // now kmin1, kmin2 and kmin3 are nearest speakers numbers and dmin1,dmin2 and dmin3 smallest distances.


  //-------------2.Calculate the inverse square matrix:--------------

  //elements of the 3x3 matrix with unit vectors of the louspeaker direction
  double L_11(spkpos[kmin1].unitvector.x);
  double L_12(spkpos[kmin1].unitvector.y);
  double L_13(spkpos[kmin1].unitvector.z);
  double L_21(spkpos[kmin2].unitvector.x);
  double L_22(spkpos[kmin2].unitvector.y);
  double L_23(spkpos[kmin2].unitvector.z);
  double L_31(spkpos[kmin3].unitvector.x);
  double L_32(spkpos[kmin3].unitvector.y);
  double L_33(spkpos[kmin3].unitvector.z);
  
  // matrix with cofactors
  //double Co_11=(L_22*L_33)-(L_32*L_23);
  //double Co_12=(L_21*L_33)-(L_31*L_23)*(-1);
  //double Co_13=(L_21*L_32)-(L_31*L_32);
  //double Co_21=(L_12*L_33)-(L_32*L_13)*(-1);
  //double Co_22=(L_11*L_33)-(L_31*L_13);
  //double Co_23=(L_11*L_32)-(L_31*L_12)*(-1);
  //double Co_31=(L_12*L_23)-(L_22*L_13);
  //double Co_32=(L_11*L_23)-(L_21*L_13)*(-1);
  //double Co_33=(L_11*L_22)-(L_21*L_12);

  // adjoint matrix - transpose of the cofactor matrix Co_ij=Adj_ji
  double Adj_11=(L_22*L_33)-(L_32*L_23);
  double Adj_21=(L_21*L_33)-(L_31*L_23)*(-1);
  double Adj_31=(L_21*L_32)-(L_31*L_32);
  double Adj_12=(L_12*L_33)-(L_32*L_13)*(-1);
  double Adj_22=(L_11*L_33)-(L_31*L_13);
  double Adj_32=(L_11*L_32)-(L_31*L_12)*(-1);
  double Adj_13=(L_12*L_23)-(L_22*L_13);
  double Adj_23=(L_11*L_23)-(L_21*L_13)*(-1);
  double Adj_33=(L_11*L_22)-(L_21*L_12);


  //determinant of a 3x3 matrix 
  double det_inv = 1/(L_11*Adj_11+L_12*Adj_21+L_13*Adj_31);// inverse of a determinant of a matrix

  //inverse of a matrix L
  double L_inv_11 =det_inv*Adj_11;
  double L_inv_12 =det_inv*Adj_12;
  double L_inv_13 =det_inv*Adj_13;
  double L_inv_21 =det_inv*Adj_21;
  double L_inv_22 =det_inv*Adj_22;
  double L_inv_23 =det_inv*Adj_23;
  double L_inv_31 =det_inv*Adj_31;
  double L_inv_32 =det_inv*Adj_32;
  double L_inv_33 =det_inv*Adj_33;
  
  //-------------3.Calculate the gains for louspeakers according to formula [w1 w2 w3]=[p1 p2 p3]*L^-1  :--------------

  double w_kmin1= psrc_normal.x*L_inv_11+ psrc_normal.y*L_inv_21+psrc_normal.z*L_inv_31;
  double w_kmin2= psrc_normal.x*L_inv_12+ psrc_normal.y*L_inv_22+psrc_normal.z*L_inv_32;
  double w_kmin3= psrc_normal.x*L_inv_13+ psrc_normal.y*L_inv_23+psrc_normal.z*L_inv_33;

 

  //-------------4.Apply the gain into the signal  :--------------

  //We get information about the gain (w) for loudspeaker only once in each block so we know the value of the gain at the beginning  and at the end of each block (at the beginning - value from the previous frame, at the end - value calculated in this iteration). Because we dont know the exact value for each time sample, we have to linearly interpolate between the two values that we have. To do this we have to compute the increment, and then this increment is added at every sample. 

  //4.a) Compute increment (dwp) based on current value (w_kmin) and previous value (wp) and number of samples dt =1/NrSamples
  for(unsigned int k=0;k<N;k++){
    d->dwp[k] = (0 - d->wp[k])*d->dt;//dwp=(0-wp)*dt
  }

  d->dwp[kmin1] = (w_kmin1 - d->wp[kmin1])*d->dt;// (dwp=G-wp)*dt 
  d->dwp[kmin2] = (w_kmin2 - d->wp[kmin2])*d->dt;
  d->dwp[kmin3] = (w_kmin3 - d->wp[kmin3])*d->dt;
 
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

TASCAR::receivermod_base_t::data_t* rec_vbap3d_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(fragsize,spkpos.size());
}

REGISTER_RECEIVERMOD(rec_vbap3d_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
