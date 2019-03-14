#include "quickhull/QuickHull.hpp"
#include "quickhull/QuickHull.cpp"

#include "errorhandling.h"
#include "scene.h"
#include <math.h>

using namespace quickhull;


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

class rec_vbap_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t channels);
    virtual ~data_t();
    // loudspeaker driving weights:
    float* wp;
    // differential driving weights:
    float* dwp;
  };
  rec_vbap_t(xmlpp::Element* xmlsrc);
  virtual ~rec_vbap_t() {};
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
  std::vector<simplex_t> simplices;
};

rec_vbap_t::data_t::data_t( uint32_t channels )
{
  wp = new float[channels];
  dwp = new float[channels];
  for(uint32_t k=0;k<channels;++k)
    wp[k] = dwp[k] = 0;
}

rec_vbap_t::data_t::~data_t()
{
  delete [] wp;
  delete [] dwp;
}

uint32_t findindex( const std::vector<Vector3<double>>& spklist, const Vector3<double>& vertex )
{
  for(uint32_t k=0;k<spklist.size();++k)
    if( (spklist[k].x == vertex.x) &&
        (spklist[k].y == vertex.y) &&
        (spklist[k].z == vertex.z)) 
      return k;
  throw TASCAR::ErrMsg("Simplex index not found in list");
  return spklist.size();
}

bool vector_is_equal( const std::vector<IndexType>& a, const std::vector<IndexType>& b )
{
  if( a.size() != b.size() )
    return false;
  for( uint32_t k=0;k<a.size();++k)
    if( a[k] != b[k] )
      return false;
  return true;
}

rec_vbap_t::rec_vbap_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc)
{
  if( spkpos.size() < 3 )
    throw TASCAR::ErrMsg("At least three loudspeakers are required for 3D-VBAP.");
  // create a quickhull point list from speaker layout:
  std::vector<Vector3<double>> spklist;
  for(uint32_t k=0;k<spkpos.size();++k)
    spklist.push_back( Vector3<double>(spkpos[k].unitvector.x,spkpos[k].unitvector.y,spkpos[k].unitvector.z));
  // calculate the convex hull:
  QuickHull<double> qh;
  auto hull = qh.getConvexHull( spklist, true, true );
  auto indexBuffer = hull.getIndexBuffer();
  // test degenerated convex hull:
  std::vector<Vector3<double>> spklist2(spklist);
  spklist2.push_back( Vector3<double>(0,0,0) );
  spklist2.push_back( Vector3<double>(0.1,0,0) );
  spklist2.push_back( Vector3<double>(0,0.1,0) );
  spklist2.push_back( Vector3<double>(0,0,0.1) );
  QuickHull<double> qh2;
  auto hull2 = qh2.getConvexHull( spklist2, true, true );
  auto indexBuffer2 = hull2.getIndexBuffer();
  if( !vector_is_equal(indexBuffer,indexBuffer2) )
    throw TASCAR::ErrMsg("Loudspeaker layout is flat (not a 3D layout?).");
  auto vertexBuffer = hull.getVertexBuffer();
  if( indexBuffer.size() < 12 )
      throw TASCAR::ErrMsg("Invalid convex hull.");
  // identify channel numbers of simplex vertices and store inverted
  // loudspeaker matrices:
  for( uint32_t khull=0;khull<indexBuffer.size();khull+=3){
    simplex_t sim;
    sim.c1 = findindex(spklist,vertexBuffer[indexBuffer[khull]]);
    sim.c2 = findindex(spklist,vertexBuffer[indexBuffer[khull+1]]);
    sim.c3 = findindex(spklist,vertexBuffer[indexBuffer[khull+2]]);
    double l11(spklist[sim.c1].x);
    double l12(spklist[sim.c1].y);
    double l13(spklist[sim.c1].z);
    double l21(spklist[sim.c2].x);
    double l22(spklist[sim.c2].y);
    double l23(spklist[sim.c2].z);
    double l31(spklist[sim.c3].x);
    double l32(spklist[sim.c3].y);
    double l33(spklist[sim.c3].z);
    double det_speaker(l11*l22*l33 + l12*l23*l31 + l13*l21*l32 - l31*l22*l13 - l32*l23*l11 - l33*l21*l12);
    if( det_speaker != 0 )
      det_speaker = 1.0/det_speaker;
    sim.l11 = det_speaker*(l22*l33 - l23*l32);
    sim.l12 = -det_speaker*(l12*l33 - l13*l32);
    sim.l13 = det_speaker*(l12*l23 - l13*l22);
    sim.l21 = -det_speaker*(l21*l33 - l23*l31);
    sim.l22 = det_speaker*(l11*l33 - l13*l31);
    sim.l23 = -det_speaker*(l11*l23 - l13*l21);
    sim.l31 = det_speaker*(l21*l32 - l22*l31);
    sim.l32 = -det_speaker*(l11*l32 - l12*l31);
    sim.l33 = det_speaker*(l11*l22 - l12*l21);
    simplices.push_back(sim);
  }
}

/*
  See receivermod_base_t::add_pointsource() in file receivermod.h for details.
*/
void rec_vbap_t::add_pointsource( const TASCAR::pos_t& prel,
                                  double width,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>& output,
                                  receivermod_base_t::data_t* sd)
{
  // N is the number of loudspeakers:
  uint32_t N(output.size());

  // d is the internal state variable for this specific
  // receiver-source-pair:
  data_t* d((data_t*)sd);//it creates the variable d

  // psrc_normal is the normalized source direction in the receiver
  // coordinate system:
  TASCAR::pos_t psrc_normal(prel.normal());

  for(unsigned int k=0;k<N;k++)
    d->dwp[k] = - d->wp[k]*t_inc;
  uint32_t k=0;
  for( auto it=simplices.begin();it!=simplices.end();++it){
    if( it->get_gain(psrc_normal) ){
      d->dwp[it->c1] = (it->g1 - d->wp[it->c1])*t_inc;
      d->dwp[it->c2] = (it->g2 - d->wp[it->c2])*t_inc;
      d->dwp[it->c3] = (it->g3 - d->wp[it->c3])*t_inc;
    }
    ++k;
  }
  // i is time (in samples):
  for( unsigned int i=0;i<chunk.size();i++){
    // k is output channel number:
    for( unsigned int k=0;k<N;k++){
      //output in each louspeaker k at sample i:
      output[k][i] += (d->wp[k] += d->dwp[k]) * chunk[i];
      // This += is because we sum up all the sources for which we
      // call this func
    }
  }
}

TASCAR::receivermod_base_t::data_t* rec_vbap_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t(spkpos.size());
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
