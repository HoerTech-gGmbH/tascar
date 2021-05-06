/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
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

//#include "quickhull/QuickHull.hpp"
//#include "quickhull/QuickHull.cpp"

#include "vbap3d.h"
#include "errorhandling.h"

#include <string.h>

// using namespace quickhull;
using namespace TASCAR;

vbap3d_t::vbap3d_t(const std::vector<TASCAR::pos_t>& spkarray)
    : numchannels(spkarray.size()), channelweights(new float[numchannels]),
      weights(channelweights)
{
  if(spkarray.size() < 3)
    throw TASCAR::ErrMsg(
        "At least three loudspeakers are required for 3D-VBAP.");
  // create a quickhull point list from speaker layout:
  std::vector<TASCAR::pos_t> spklist;
  for(uint32_t k = 0; k < spkarray.size(); ++k)
    spklist.push_back(spkarray[k].normal());
  // calculate the convex hull:
  TASCAR::quickhull_t qh(spklist);
  std::vector<TASCAR::pos_t> spklist2(spklist);
  spklist2.push_back(TASCAR::pos_t(0, 0, 0));
  spklist2.push_back(TASCAR::pos_t(0.1, 0, 0));
  spklist2.push_back(TASCAR::pos_t(0, 0.1, 0));
  spklist2.push_back(TASCAR::pos_t(0, 0, 0.1));
  TASCAR::quickhull_t qh2(spklist2);
  if(!(qh == qh2)) {
    throw TASCAR::ErrMsg("Loudspeaker layout is flat (not a 3D layout?).");
  }
  for(auto it = qh.faces.begin(); it != qh.faces.end(); ++it) {
    simplex_t sim;
    sim.c1 = it->c1;
    sim.c2 = it->c2;
    sim.c3 = it->c3;
    double l11(spklist[sim.c1].x);
    double l12(spklist[sim.c1].y);
    double l13(spklist[sim.c1].z);
    double l21(spklist[sim.c2].x);
    double l22(spklist[sim.c2].y);
    double l23(spklist[sim.c2].z);
    double l31(spklist[sim.c3].x);
    double l32(spklist[sim.c3].y);
    double l33(spklist[sim.c3].z);
    double det_speaker(l11 * l22 * l33 + l12 * l23 * l31 + l13 * l21 * l32 -
                       l31 * l22 * l13 - l32 * l23 * l11 - l33 * l21 * l12);
    if(det_speaker != 0)
      det_speaker = 1.0 / det_speaker;
    sim.l11 = det_speaker * (l22 * l33 - l23 * l32);
    sim.l12 = -det_speaker * (l12 * l33 - l13 * l32);
    sim.l13 = det_speaker * (l12 * l23 - l13 * l22);
    sim.l21 = -det_speaker * (l21 * l33 - l23 * l31);
    sim.l22 = det_speaker * (l11 * l33 - l13 * l31);
    sim.l23 = -det_speaker * (l11 * l23 - l13 * l21);
    sim.l31 = det_speaker * (l21 * l32 - l22 * l31);
    sim.l32 = -det_speaker * (l11 * l32 - l12 * l31);
    sim.l33 = det_speaker * (l11 * l22 - l12 * l21);
    simplices.push_back(sim);
  }
}

vbap3d_t::~vbap3d_t()
{
  delete[] channelweights;
}

void vbap3d_t::encode(const TASCAR::pos_t& prelnorm)
{
  encode(prelnorm, channelweights);
}

void vbap3d_t::encode(const TASCAR::pos_t& prelnorm, float* weights) const
{
  memset(weights, 0, sizeof(float) * numchannels);
  for(auto it = simplices.begin(); it != simplices.end(); ++it) {
    float g1(0.0f);
    float g2(0.0f);
    float g3(0.0f);
    if(it->get_gain(prelnorm, g1, g2, g3)) {
      weights[it->c1] = g1;
      weights[it->c2] = g2;
      weights[it->c3] = g3;
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
