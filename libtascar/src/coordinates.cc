/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/**
 * @file coordinates.cc
 * @brief "coordinates" provide classes for coordinate handling
 * @ingroup libtascar
 * @author Giso Grimm
 * @date 2012
 *
 * @section license License (GPL)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include "coordinates.h"
#include "errorhandling.h"
#include "tscconfig.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define QUICKHULL_1

#ifdef QUICKHULL_1
#include "quickhull/QuickHull.cpp"
#include "quickhull/QuickHull.hpp"
using namespace quickhull;
#else
#define CONVHULL_3D_ENABLE
#include "convhull_3d/convhull_3d.h"
#endif

using namespace TASCAR;

double TASCAR::drand()
{
  return (double)rand() / (double)(RAND_MAX + 1.0);
}

std::string pos_t::print_cart(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(12);
  tmp << x << delim << y << delim << z;
  return tmp.str();
}

std::string pos_t::print_sphere(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(12);
  tmp << norm() << delim << RAD2DEG * azim() << delim << RAD2DEG * elev();
  return tmp.str();
}

std::string posf_t::print_cart(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(9);
  tmp << x << delim << y << delim << z;
  return tmp.str();
}

std::string posf_t::print_sphere(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(9);
  tmp << norm() << delim << RAD2DEGf * azim() << delim << RAD2DEGf * elev();
  return tmp.str();
}

table1_t::table1_t() {}

double table1_t::interp(double x) const
{
  if(begin() == end())
    return 0;
  const_iterator lim2 = lower_bound(x);
  if(lim2 == end())
    return rbegin()->second;
  if(lim2 == begin())
    return begin()->second;
  if(lim2->first == x)
    return lim2->second;
  const_iterator lim1 = lim2;
  --lim1;
  // cartesian interpolation:
  double p1(lim1->second);
  double p2(lim2->second);
  double w = (x - lim1->first) / (lim2->first - lim1->first);
  make_friendly_number(w);
  p1 *= (1.0 - w);
  p2 *= w;
  p1 += p2;
  return p1;
}

void pos_t::normalize()
{
  *this /= norm();
}

void posf_t::normalize()
{
  *this /= norm();
}

bool pos_t::has_infinity() const
{
  if(x == std::numeric_limits<double>::infinity())
    return true;
  if(y == std::numeric_limits<double>::infinity())
    return true;
  if(z == std::numeric_limits<double>::infinity())
    return true;
  return false;
}

bool posf_t::has_infinity() const
{
  if(x == std::numeric_limits<float>::infinity())
    return true;
  if(y == std::numeric_limits<float>::infinity())
    return true;
  if(z == std::numeric_limits<float>::infinity())
    return true;
  return false;
}

shoebox_t::shoebox_t() {}

shoebox_t::shoebox_t(const pos_t& center_, const pos_t& size_,
                     const zyx_euler_t& orientation_)
    : center(center_), size(size_), orientation(orientation_)
{
}

pos_t shoebox_t::nextpoint(pos_t p)
{
  p -= center;
  p /= orientation;
  pos_t prel;
  if(p.x > 0)
    prel.x = std::max(0.0, p.x - 0.5 * size.x);
  else
    prel.x = std::min(0.0, p.x + 0.5 * size.x);
  if(p.y > 0)
    prel.y = std::max(0.0, p.y - 0.5 * size.y);
  else
    prel.y = std::min(0.0, p.y + 0.5 * size.y);
  if(p.z > 0)
    prel.z = std::max(0.0, p.z - 0.5 * size.z);
  else
    prel.z = std::min(0.0, p.z + 0.5 * size.z);
  return prel;
}

ngon_t& ngon_t::operator+=(const pos_t& p)
{
  delta += p;
  update();
  return *this;
}

ngon_t& ngon_t::operator+=(double p)
{
  pos_t n(normal);
  n *= p;
  return (*this += n);
}

ngon_t::ngon_t() : N(4)
{
  nonrt_set_rect(1, 2);
}

void ngon_t::nonrt_set_rect(double width, double height)
{
  std::vector<pos_t> nverts;
  nverts.push_back(pos_t(0, 0, 0));
  nverts.push_back(pos_t(0, width, 0));
  nverts.push_back(pos_t(0, width, height));
  nverts.push_back(pos_t(0, 0, height));
  nonrt_set(nverts);
}

void ngon_t::nonrt_set(const std::vector<pos_t>& verts)
{
  if(verts.size() < 3)
    throw TASCAR::ErrMsg("A polygon needs at least three vertices.");
  if(verts.size() > (size_t)1 << 31)
    throw TASCAR::ErrMsg("Too many vertices.");
  local_verts_ = verts;
  N = (uint32_t)(verts.size());
  verts_.resize(N);
  edges_.resize(N);
  vert_normals_.resize(N);
  edge_normals_.resize(N);
  // calculate area and aperture size:
  pos_t rot;
  std::vector<pos_t>::iterator i_prev_vert(local_verts_.end() - 1);
  for(std::vector<pos_t>::iterator i_vert = local_verts_.begin();
      i_vert != local_verts_.end(); ++i_vert) {
    rot += cross_prod(*i_prev_vert, *i_vert);
    i_prev_vert = i_vert;
  }
  local_normal = rot;
  local_normal /= local_normal.norm();
  area = 0.5 * rot.norm();
  aperture = 2.0 * sqrt(area / TASCAR_PI);
  // update global coordinates:
  update();
}

void ngon_t::apply_rot_loc(const pos_t& p0, const zyx_euler_t& o)
{
  orientation = o;
  delta = p0;
  update();
}

void ngon_t::update()
{
  // firtst calculate vertices in global coordinate system:
  std::vector<pos_t>::iterator i_local_vert(local_verts_.begin());
  for(std::vector<pos_t>::iterator i_vert = verts_.begin();
      i_vert != verts_.end(); ++i_vert) {
    *i_vert = *i_local_vert;
    *i_vert *= orientation;
    *i_vert += delta;
    ++i_local_vert;
  }
  std::vector<pos_t>::iterator i_vert(verts_.begin());
  std::vector<pos_t>::iterator i_next_vert(i_vert + 1);
  for(std::vector<pos_t>::iterator i_edge = edges_.begin();
      i_edge != edges_.end(); ++i_edge) {
    *i_edge = *i_next_vert;
    *i_edge -= *i_vert;
    ++i_next_vert;
    if(i_next_vert == verts_.end())
      i_next_vert = verts_.begin();
    ++i_vert;
  }
  normal = local_normal;
  normal *= orientation;
  // vertex normals, used to calculate inside/outside:
  std::vector<pos_t>::iterator i_prev_edge(edges_.end() - 1);
  std::vector<pos_t>::iterator i_edge(edges_.begin());
  for(std::vector<pos_t>::iterator i_vert_normal = vert_normals_.begin();
      i_vert_normal != vert_normals_.end(); ++i_vert_normal) {
    *i_vert_normal =
        cross_prod(i_edge->normal() + i_prev_edge->normal(), normal).normal();
    i_prev_edge = i_edge;
    ++i_edge;
  }
  for(uint32_t k = 0; k < N; ++k) {
    edge_normals_[k] = cross_prod(edges_[k].normal(), normal);
  }
}

pos_t ngon_t::nearest_on_plane(const pos_t& p0) const
{
  double plane_dist = dot_prod(normal, verts_[0] - p0);
  pos_t p0d = normal;
  p0d *= plane_dist;
  p0d += p0;
  return p0d;
}

pos_t TASCAR::edge_nearest(const pos_t& v, const pos_t& d, const pos_t& p0)
{
  pos_t p0p1(p0 - v);
  double l(d.norm());
  pos_t n(d);
  n /= l;
  double r(0.0);
  if(!p0p1.is_null())
    r = dot_prod(n, p0p1.normal()) * p0p1.norm();
  if(r < 0)
    return v;
  if(r > l)
    return v + d;
  pos_t p0d(n);
  p0d *= r;
  p0d += v;
  return p0d;
}

pos_t ngon_t::nearest_on_edge(const pos_t& p0, uint32_t* pk0) const
{
  pos_t ne(edge_nearest(verts_[0], edges_[0], p0));
  double d(distance(ne, p0));
  uint32_t k0(0);
  for(uint32_t k = 1; k < N; k++) {
    pos_t ln(edge_nearest(verts_[k], edges_[k], p0));
    double ld;
    if((ld = distance(ln, p0)) < d) {
      ne = ln;
      d = ld;
      k0 = k;
    }
  }
  if(pk0)
    *pk0 = k0;
  return ne;
}

pos_t ngon_t::nearest(const pos_t& p0, bool* is_outside_, pos_t* on_edge_) const
{
  uint32_t k0(0);
  pos_t ne(nearest_on_edge(p0, &k0));
  if(on_edge_)
    *on_edge_ = ne;
  // is inside?
  bool is_outside(false);
  pos_t dp0(ne - p0);
  if(dp0.is_null())
    // point is exactly on the edge
    is_outside = true;
  else
    // caclulate edge normal:
    is_outside = (dot_prod(edge_normals_[k0], dp0) < 0);
  if(is_outside_)
    *is_outside_ = is_outside;
  if(is_outside)
    return ne;
  return nearest_on_plane(p0);
}

bool ngon_t::is_infront(const pos_t& p0) const
{
  pos_t p_cut(nearest_on_plane(p0));
  return (dot_prod(p0 - p_cut, normal) > 0);
}

bool ngon_t::is_behind(const pos_t& p0) const
{
  pos_t p_cut(nearest_on_plane(p0));
  return (dot_prod(p0 - p_cut, normal) < 0);
}

bool ngon_t::intersection(const pos_t& p0, const pos_t& p1, pos_t& p_is,
                          double* w) const
{
  pos_t np(nearest_on_plane(p0));
  pos_t dpn(p1 - p0);
  double dpl(dpn.norm());
  dpn.normalize();
  double d(distance(np, p0));
  if(d == 0) {
    // first point is intersecting:
    if(w)
      *w = 0;
    p_is = p0;
    return true;
  }
  double r(dot_prod(dpn, (np - p0).normal()));
  if(r == 0) {
    // the edge is parallel to the plane; no intersection.
    return false;
  }
  r = d / r;
  dpn *= r;
  r /= dpl;
  if(w)
    *w = r;
  dpn += p0;
  p_is = dpn;
  return true;
}

std::string ngon_t::print(const std::string& delim) const
{
  std::ostringstream tmp("");
  tmp.precision(12);
  for(std::vector<pos_t>::const_iterator i_vert = verts_.begin();
      i_vert != verts_.end(); ++i_vert) {
    if(i_vert != verts_.begin())
      tmp << delim;
    tmp << i_vert->print_cart(delim);
  }
  return tmp.str();
}

std::ostream& operator<<(std::ostream& out, const TASCAR::pos_t& p)
{
  out << p.print_cart();
  return out;
}

std::ostream& operator<<(std::ostream& out, const TASCAR::ngon_t& n)
{
  out << n.print();
  return out;
}

std::vector<pos_t> TASCAR::generate_icosahedron()
{
  std::vector<pos_t> m;
  double phi((1.0 + sqrt(5.0)) / 2.0);
  m.push_back(pos_t(0, 1, phi));
  m.push_back(pos_t(0, -1, -phi));
  m.push_back(pos_t(0, 1, -phi));
  m.push_back(pos_t(0, -1, phi));
  m.push_back(pos_t(1, phi, 0));
  m.push_back(pos_t(1, -phi, 0));
  m.push_back(pos_t(-1, -phi, 0));
  m.push_back(pos_t(-1, phi, 0));
  m.push_back(pos_t(phi, 0, 1));
  m.push_back(pos_t(-phi, 0, 1));
  m.push_back(pos_t(phi, 0, -1));
  m.push_back(pos_t(-phi, 0, -1));
  return m;
}

#ifdef QUICKHULL_1
uint32_t findindex2(const std::vector<Vector3<double>>& spklist,
                    const Vector3<double>& vertex)
{
  for(uint32_t k = 0; k < spklist.size(); ++k)
    if((spklist[k].x == vertex.x) && (spklist[k].y == vertex.y) &&
       (spklist[k].z == vertex.z))
      return k;
  throw TASCAR::ErrMsg("Simplex index not found in list");
  return (uint32_t)(spklist.size());
}

TASCAR::quickhull_t::quickhull_t(const std::vector<pos_t>& mesh)
{
  std::vector<Vector3<double>> spklist;
  for(auto it = mesh.begin(); it != mesh.end(); ++it)
    spklist.push_back(Vector3<double>(it->x, it->y, it->z));
  QuickHull<double> qh;
  auto hull = qh.getConvexHull(spklist, true, true);
  auto indexBuffer = hull.getIndexBuffer();
  if(indexBuffer.size() < 12)
    throw TASCAR::ErrMsg("Invalid convex hull.");
  auto vertexBuffer = hull.getVertexBuffer();
  for(uint32_t khull = 0; khull < indexBuffer.size(); khull += 3) {
    simplex_t sim;
    sim.c1 = findindex2(spklist, vertexBuffer[indexBuffer[khull]]);
    sim.c2 = findindex2(spklist, vertexBuffer[indexBuffer[khull + 1]]);
    sim.c3 = findindex2(spklist, vertexBuffer[indexBuffer[khull + 2]]);
    faces.push_back(sim);
  }
}
#else

uint32_t findindex2(const std::vector<TASCAR::pos_t>& spklist,
                    const ch_vertex& vertex)
{
  for(uint32_t k = 0; k < spklist.size(); ++k)
    if((spklist[k].x == vertex.x) && (spklist[k].y == vertex.y) &&
       (spklist[k].z == vertex.z))
      return k;
  throw TASCAR::ErrMsg("Simplex index not found in list");
  return spklist.size();
}

TASCAR::quickhull_t::quickhull_t(const std::vector<pos_t>& mesh)
{
  ch_vertex* vertices;
  vertices = (ch_vertex*)malloc(mesh.size() * sizeof(ch_vertex));
  size_t k(0);
  for(auto it = mesh.begin(); it != mesh.end(); ++it) {
    vertices[k].x = it->x;
    vertices[k].y = it->y;
    vertices[k].z = it->z;
    ++k;
  }
  int* faceIndices(NULL);
  int nFaces(0);
  convhull_3d_build(vertices, mesh.size(), &faceIndices, &nFaces);
  for(int khull = 0; khull < nFaces; ++khull) {
    simplex_t sim;
    // sim.c1 = faceIndices[khull];
    // sim.c2 = faceIndices[khull+nFaces];
    // sim.c3 = faceIndices[khull+2*nFaces];
    sim.c1 = faceIndices[3 * khull];
    sim.c2 = faceIndices[3 * khull + 1];
    sim.c3 = faceIndices[3 * khull + 2];
    sim.c1 = findindex2(mesh, vertices[faceIndices[3 * khull]]);
    sim.c2 = findindex2(mesh, vertices[faceIndices[3 * khull + 1]]);
    sim.c3 = findindex2(mesh, vertices[faceIndices[3 * khull + 2]]);
    faces.push_back(sim);
  }
  free(vertices);
  free(faceIndices);
}
#endif

std::vector<pos_t> TASCAR::subdivide_and_normalize_mesh(std::vector<pos_t> mesh,
                                                        uint32_t iterations)
{
  for(auto it = mesh.begin(); it != mesh.end(); ++it)
    it->normalize();
  for(uint32_t i = 0; i < iterations; ++i) {
    TASCAR::quickhull_t qh(mesh);
    for(auto it = qh.faces.begin(); it != qh.faces.end(); ++it) {
      pos_t pn(mesh[it->c1]);
      pn += mesh[it->c2];
      pn += mesh[it->c3];
      pn *= 1.0 / 3.0;
      mesh.push_back(pn);
    }
    for(auto it = mesh.begin(); it != mesh.end(); ++it)
      it->normalize();
  }
  return mesh;
}

bool operator==(const TASCAR::quickhull_t& h1, const TASCAR::quickhull_t& h2)
{
  if(h1.faces.size() != h2.faces.size())
    return false;
  // see if all faces are also available in other mesh:
  uint32_t k = 0;
  for(auto it = h1.faces.begin(); it != h1.faces.end(); ++it) {
    bool found(false);
    for(auto it2 = h2.faces.begin(); it2 != h2.faces.end(); ++it2)
      if(*it == *it2)
        found = true;
    if(!found) {
      return false;
    }
    ++k;
  }
  return true;
}

bool operator==(const TASCAR::quickhull_t::simplex_t& s1,
                const TASCAR::quickhull_t::simplex_t& s2)
{
  return (s1.c1 == s2.c1) && (s1.c2 == s2.c2) && (s1.c3 == s2.c3);
}

std::string TASCAR::to_string(const rotmat_t& m)
{
  std::string s =
      "\n[" + to_string(m.m11, "%1.4g") + " " + to_string(m.m12, "%1.4g") +
      " " + to_string(m.m13, "%1.4g") + "]\n[" + to_string(m.m21, "%1.4g") +
      " " + to_string(m.m22, "%1.4g") + " " + to_string(m.m23, "%1.4g") +
      "]\n[" + to_string(m.m31, "%1.4g") + " " + to_string(m.m32, "%1.4g") +
      " " + to_string(m.m33, "%1.4g") + "]\n";
  return s;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
