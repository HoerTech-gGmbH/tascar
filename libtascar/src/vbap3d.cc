#include "quickhull/QuickHull.hpp"
#include "quickhull/QuickHull.cpp"

#include "errorhandling.h"
#include "vbap3d.h"

using namespace quickhull;
using namespace TASCAR;

bool vector_is_equal( const std::vector<IndexType>& a, const std::vector<IndexType>& b )
{
  if( a.size() != b.size() )
    return false;
  for( uint32_t k=0;k<a.size();++k)
    if( a[k] != b[k] )
      return false;
  return true;
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

vbap3d_t::vbap3d_t(const std::vector<spk_descriptor_t>& spkarray)
  : numchannels(spkarray.size()),
    channelweights(new float[numchannels]),
    weights(channelweights)
{
  if( spkarray.size() < 3 )
    throw TASCAR::ErrMsg("At least three loudspeakers are required for 3D-VBAP.");
  // create a quickhull point list from speaker layout:
  std::vector<Vector3<double>> spklist;
  for(uint32_t k=0;k<spkarray.size();++k)
    spklist.push_back( Vector3<double>(spkarray[k].unitvector.x,spkarray[k].unitvector.y,spkarray[k].unitvector.z));
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

vbap3d_t::~vbap3d_t()
{
  delete [] channelweights;
}

void vbap3d_t::encode( const TASCAR::pos_t& prelnorm )
{
  //DEBUG(prelnorm.print_cart());
  memset(channelweights,0,sizeof(float)*numchannels);
  for( auto it=simplices.begin();it!=simplices.end();++it){
    if( it->get_gain(prelnorm) ){
      channelweights[it->c1] = it->g1;
      channelweights[it->c2] = it->g2;
      channelweights[it->c3] = it->g3;
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
