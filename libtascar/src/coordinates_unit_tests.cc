/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include <gtest/gtest.h>

#include "coordinates.h"

TEST(table1_t,interp)
{
  TASCAR::table1_t tab;
  EXPECT_EQ(0u,tab.size());
  tab[1.0] = 2.0;
  EXPECT_EQ(2.0,tab.interp(0.0));
  EXPECT_EQ(2.0,tab.interp(5.0));
  tab[3.0] = 1.0;
  EXPECT_EQ(2.0,tab.interp(0.0));
  EXPECT_EQ(2.0,tab.interp(0.5));
  EXPECT_EQ(2.0,tab.interp(1.0));
  EXPECT_EQ(1.5,tab.interp(2.0));
  EXPECT_EQ(1.25,tab.interp(2.5));
  EXPECT_EQ(1.0,tab.interp(3.0));
  EXPECT_EQ(1.0,tab.interp(3.5));
  EXPECT_EQ(1.0,tab.interp(4.0));
  tab[2.0] = 2.0;
  EXPECT_EQ(2.0,tab.interp(0.0));
  EXPECT_EQ(2.0,tab.interp(0.5));
  EXPECT_EQ(2.0,tab.interp(1.0));
  EXPECT_EQ(2.0,tab.interp(2.0));
  EXPECT_EQ(1.5,tab.interp(2.5));
  EXPECT_EQ(1.0,tab.interp(3.0));
  EXPECT_EQ(1.0,tab.interp(3.5));
  EXPECT_EQ(1.0,tab.interp(4.0));
}

TEST(pos_t,set_sphere)
{
  TASCAR::pos_t pos;
  EXPECT_EQ(0.0,pos.x);
  EXPECT_EQ(0.0,pos.y);
  EXPECT_EQ(0.0,pos.z);
  pos.set_sphere(1.0,0.0,0.0);
  EXPECT_EQ(1.0,pos.x);
  EXPECT_EQ(0.0,pos.y);
  EXPECT_EQ(0.0,pos.z);
  pos.set_sphere(2.0,0.0,0.0);
  EXPECT_EQ(2.0,pos.x);
  EXPECT_EQ(0.0,pos.y);
  EXPECT_EQ(0.0,pos.z);
  pos.set_sphere(1.0,TASCAR_PI,0.0);
  ASSERT_NEAR(-1.0,pos.x,1e-10);
  ASSERT_NEAR(0.0,pos.y,1e-10);
  ASSERT_NEAR(0.0,pos.z,1e-10);
  pos.set_sphere(1.0,TASCAR_PI,TASCAR_PI2);
  ASSERT_NEAR(0.0,pos.x,1e-10);
  ASSERT_NEAR(0.0,pos.y,1e-10);
  ASSERT_NEAR(1.0,pos.z,1e-10);
  pos.set_sphere(1.0,TASCAR_PI,-0.25*TASCAR_PI);
  ASSERT_NEAR(-0.7071068,pos.x,1e-7);
  ASSERT_NEAR(0.0,pos.y,1e-10);
  ASSERT_NEAR(-0.7071068,pos.z,1e-7);
  pos.set_sphere(1.0,TASCAR_PI2,-0.25*TASCAR_PI);
  ASSERT_NEAR(0.0,pos.x,1e-7);
  ASSERT_NEAR(0.7071068,pos.y,1e-7);
  ASSERT_NEAR(-0.7071068,pos.z,1e-7);
}

TEST(pos_t,norm2)
{
  TASCAR::pos_t pos(1.0,0.0,0.0);
  EXPECT_EQ(1.0,pos.norm2());
  pos.x = 3.0;
  pos.y = 4.0;
  EXPECT_EQ(25.0,pos.norm2());
  pos.x = 0.0;
  pos.y = 4.0;
  pos.z = 3.0;
  EXPECT_EQ(25.0,pos.norm2());
}

TEST(pos_t,norm)
{
  TASCAR::pos_t pos(1.0,0.0,0.0);
  EXPECT_EQ(1.0,pos.norm());
  pos.x = 3.0;
  pos.y = 4.0;
  EXPECT_EQ(5.0,pos.norm());
  pos.x = 0.0;
  pos.y = 4.0;
  pos.z = 3.0;
  EXPECT_EQ(5.0,pos.norm());
}

TEST(pos_t,norm_xy)
{
  TASCAR::pos_t pos(1.0,0.0,0.0);
  EXPECT_EQ(1.0,pos.norm_xy());
  pos.x = 3.0;
  pos.y = 4.0;
  EXPECT_EQ(5.0,pos.norm_xy());
  pos.x = 0.0;
  pos.y = 4.0;
  pos.z = 3.0;
  EXPECT_EQ(4.0,pos.norm_xy());
}

TEST(ngon_t,nonrt_set_rect)
{
  TASCAR::ngon_t rect;
  rect.nonrt_set_rect( 2.0, 1.0 );
  std::vector<TASCAR::pos_t> verts(rect.get_verts());
  // verify that vertices are correct:
  EXPECT_EQ(4u,verts.size());
  EXPECT_EQ(0.0,verts[0].x);
  EXPECT_EQ(0.0,verts[0].y);
  EXPECT_EQ(0.0,verts[0].z);
  //
  EXPECT_EQ(0.0,verts[1].x);
  EXPECT_EQ(2.0,verts[1].y);
  EXPECT_EQ(0.0,verts[1].z);
  //
  EXPECT_EQ(0.0,verts[2].x);
  EXPECT_EQ(2.0,verts[2].y);
  EXPECT_EQ(1.0,verts[2].z);
  //
  EXPECT_EQ(0.0,verts[3].x);
  EXPECT_EQ(0.0,verts[3].y);
  EXPECT_EQ(1.0,verts[3].z);
  // test face normal:
  TASCAR::pos_t normal(rect.get_normal());
  EXPECT_EQ(1.0,normal.x);
  EXPECT_EQ(0.0,normal.y);
  EXPECT_EQ(0.0,normal.z);
  // test edges:
  std::vector<TASCAR::pos_t> edges(rect.get_edges());
  EXPECT_EQ(4u,edges.size());
  EXPECT_EQ(0.0,edges[0].x);
  EXPECT_EQ(2.0,edges[0].y);
  EXPECT_EQ(0.0,edges[0].z);
  //
  EXPECT_EQ(0.0,edges[1].x);
  EXPECT_EQ(0.0,edges[1].y);
  EXPECT_EQ(1.0,edges[1].z);
  //
  EXPECT_EQ(0.0,edges[2].x);
  EXPECT_EQ(-2.0,edges[2].y);
  EXPECT_EQ(0.0,edges[2].z);
  //
  EXPECT_EQ(0.0,edges[3].x);
  EXPECT_EQ(0.0,edges[3].y);
  EXPECT_EQ(-1.0,edges[3].z);
  // test vertex normals:
  std::vector<TASCAR::pos_t> vn(rect.get_vert_normals());
  EXPECT_EQ(4u,vn.size());
  EXPECT_EQ(0.0,vn[0].x);
  ASSERT_NEAR(-sqrt(0.5),vn[0].y,1e-9);
  ASSERT_NEAR(-sqrt(0.5),vn[0].z,1e-9);
  //
  EXPECT_EQ(0.0,vn[1].x);
  ASSERT_NEAR(sqrt(0.5),vn[1].y,1e-9);
  ASSERT_NEAR(-sqrt(0.5),vn[1].z,1e-9);
  //
  EXPECT_EQ(0.0,vn[2].x);
  ASSERT_NEAR(sqrt(0.5),vn[2].y,1e-9);
  ASSERT_NEAR(sqrt(0.5),vn[2].z,1e-9);
  //
  EXPECT_EQ(0.0,vn[3].x);
  ASSERT_NEAR(-sqrt(0.5),vn[3].y,1e-9);
  ASSERT_NEAR(sqrt(0.5),vn[3].z,1e-9);
  // test edge normals:
  std::vector<TASCAR::pos_t> en(rect.get_edge_normals());
  EXPECT_EQ(4u,en.size());
  EXPECT_EQ(0.0,en[0].x);
  ASSERT_NEAR(0.0,en[0].y,1e-9);
  ASSERT_NEAR(-1,en[0].z,1e-9);
  //
  EXPECT_EQ(0.0,en[1].x);
  ASSERT_NEAR(1.0,en[1].y,1e-9);
  ASSERT_NEAR(0.0,en[1].z,1e-9);
  //
  EXPECT_EQ(0.0,en[2].x);
  ASSERT_NEAR(0.0,en[2].y,1e-9);
  ASSERT_NEAR(1.0,en[2].z,1e-9);
  //
  EXPECT_EQ(0.0,en[3].x);
  ASSERT_NEAR(-1.0,en[3].y,1e-9);
  ASSERT_NEAR( 0.0,en[3].z,1e-9);
  // test translation
  rect += TASCAR::pos_t( 1, 0, -1 );
  verts = rect.get_verts();
  // verify that vertices are correct:
  EXPECT_EQ(4u,verts.size());
  EXPECT_EQ(1.0,verts[0].x);
  EXPECT_EQ(0.0,verts[0].y);
  EXPECT_EQ(-1.0,verts[0].z);
  //
  EXPECT_EQ(1.0,verts[1].x);
  EXPECT_EQ(2.0,verts[1].y);
  EXPECT_EQ(-1.0,verts[1].z);
  //
  EXPECT_EQ(1.0,verts[2].x);
  EXPECT_EQ(2.0,verts[2].y);
  EXPECT_EQ(0.0,verts[2].z);
  //
  EXPECT_EQ(1.0,verts[3].x);
  EXPECT_EQ(0.0,verts[3].y);
  EXPECT_EQ(0.0,verts[3].z);
}

TEST(edge_t,edge_nearest)
{
  // origin:
  TASCAR::pos_t v(0,0,0);
  // direction:
  TASCAR::pos_t d(1,1,1);
  TASCAR::pos_t en;
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(0,0,0));
  EXPECT_EQ(0,en.x);
  EXPECT_EQ(0,en.y);
  EXPECT_EQ(0,en.z);
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(0.5,0.5,0.5));
  ASSERT_NEAR(0.5,en.x,1e-9);
  ASSERT_NEAR(0.5,en.y,1e-9);
  ASSERT_NEAR(0.5,en.z,1e-9);
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(-0.5,-0.5,-0.5));
  EXPECT_EQ(0.0,en.x);
  EXPECT_EQ(0.0,en.y);
  EXPECT_EQ(0.0,en.z);
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(1.5,1.5,1.5));
  EXPECT_EQ(1.0,en.x);
  EXPECT_EQ(1.0,en.y);
  EXPECT_EQ(1.0,en.z);
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(-0.5,-0.5,0));
  EXPECT_EQ(0.0,en.x);
  EXPECT_EQ(0.0,en.y);
  EXPECT_EQ(0.0,en.z);
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(0.0,0.0,10.5));
  EXPECT_EQ(1.0,en.x);
  EXPECT_EQ(1.0,en.y);
  EXPECT_EQ(1.0,en.z);
  d = TASCAR::pos_t(1,1,0);
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(0,1,0));
  ASSERT_NEAR(0.5,en.x,1e-9);
  ASSERT_NEAR(0.5,en.y,1e-9);
  ASSERT_NEAR(0,en.z,1e-9);
  en = TASCAR::edge_nearest(v,d,TASCAR::pos_t(1,0,0));
  ASSERT_NEAR(0.5,en.x,1e-9);
  ASSERT_NEAR(0.5,en.y,1e-9);
  ASSERT_NEAR(0,en.z,1e-9);
}

TEST(ngon_t,nearest_on_edge)
{
  TASCAR::ngon_t rect;
  rect.nonrt_set_rect( 2.0, 1.0 );
  uint32_t k0(0);
  TASCAR::pos_t pe;
  pe = rect.nearest_on_edge(TASCAR::pos_t(0,0,0),&k0);
  EXPECT_EQ(0u,k0);
  EXPECT_EQ(0.0,pe.x);
  EXPECT_EQ(0.0,pe.y);
  EXPECT_EQ(0.0,pe.z);
  pe = rect.nearest_on_edge(TASCAR::pos_t(0,-1,0.5),&k0);
  EXPECT_EQ(3u,k0);
  EXPECT_EQ(0.0,pe.x);
  EXPECT_EQ(0.0,pe.y);
  EXPECT_EQ(0.5,pe.z);
  pe = rect.nearest_on_edge(TASCAR::pos_t(1,-1,0.5),&k0);
  EXPECT_EQ(3u,k0);
  EXPECT_EQ(0.0,pe.x);
  EXPECT_EQ(0.0,pe.y);
  EXPECT_EQ(0.5,pe.z);
  pe = rect.nearest_on_edge(TASCAR::pos_t(1,-1,1.5),&k0);
  EXPECT_EQ(2u,k0);
  EXPECT_EQ(0.0,pe.x);
  EXPECT_EQ(0.0,pe.y);
  EXPECT_EQ(1.0,pe.z);
  pe = rect.nearest_on_edge(TASCAR::pos_t(1,-1,-1.5),&k0);
  EXPECT_EQ(0u,k0);
  EXPECT_EQ(0.0,pe.x);
  EXPECT_EQ(0.0,pe.y);
  EXPECT_EQ(0.0,pe.z);
  pe = rect.nearest_on_edge(TASCAR::pos_t(1,3,0.5),&k0);
  EXPECT_EQ(1u,k0);
  EXPECT_EQ(0.0,pe.x);
  EXPECT_EQ(2.0,pe.y);
  EXPECT_EQ(0.5,pe.z);
  // this point is in the middle, return the middle of first edge!
  pe = rect.nearest_on_edge(TASCAR::pos_t(0,1,0.5),&k0);
  EXPECT_EQ(0u,k0);
  EXPECT_EQ(0.0,pe.x);
  EXPECT_EQ(1.0,pe.y);
  EXPECT_EQ(0.0,pe.z);
}

TEST(ngon_t,nearest_on_plane)
{
  TASCAR::ngon_t rect;
  rect.nonrt_set_rect( 2.0, 1.0 );
  rect += TASCAR::pos_t( 1, 0, -1 );
  bool outside(false);
  TASCAR::pos_t on_edge;
  TASCAR::pos_t nearest;
  nearest = rect.nearest(TASCAR::pos_t(0,0,0), &outside, &on_edge);
  // this should be on edge:
  EXPECT_EQ(0,outside);
  EXPECT_EQ(1.0,nearest.x);
  EXPECT_EQ(0.0,nearest.y);
  EXPECT_EQ(0.0,nearest.z);
  EXPECT_EQ(1.0,on_edge.x);
  EXPECT_EQ(0.0,on_edge.y);
  EXPECT_EQ(0.0,on_edge.z);
  nearest = rect.nearest(TASCAR::pos_t(0,0,-0.5), &outside, &on_edge);
  // this should be on edge:
  EXPECT_EQ(0,outside);
  EXPECT_EQ(1.0,nearest.x);
  EXPECT_EQ(0.0,nearest.y);
  EXPECT_EQ(-0.5,nearest.z);
  EXPECT_EQ(1.0,on_edge.x);
  EXPECT_EQ(0.0,on_edge.y);
  EXPECT_EQ(-0.5,on_edge.z);
  nearest = rect.nearest(TASCAR::pos_t(0,0,1), &outside, &on_edge);
  // this should be outside, on top edge (1,0,0)
  EXPECT_EQ(1,outside);
  EXPECT_EQ(1.0,nearest.x);
  EXPECT_EQ(0.0,nearest.y);
  EXPECT_EQ(0.0,nearest.z);
  EXPECT_EQ(1.0,on_edge.x);
  EXPECT_EQ(0.0,on_edge.y);
  EXPECT_EQ(0.0,on_edge.z);
  nearest = rect.nearest(TASCAR::pos_t(0,0.5,1), &outside, &on_edge);
  // this should be outside, on top edge (1,0,0)
  EXPECT_EQ(1,outside);
  EXPECT_EQ(1.0,nearest.x);
  ASSERT_NEAR(0.5,nearest.y,1e-9);
  EXPECT_EQ(0.0,nearest.z);
  EXPECT_EQ(1.0,on_edge.x);
  ASSERT_NEAR(0.5,on_edge.y,1e-9);
  EXPECT_EQ(0.0,on_edge.z);
  nearest = rect.nearest(TASCAR::pos_t(0,-1,1), &outside, &on_edge);
  // this should be outside, on corner vertex (1,0,0):
  EXPECT_EQ(1,outside);
  EXPECT_EQ(1.0,nearest.x);
  EXPECT_EQ(0.0,nearest.y);
  EXPECT_EQ(0.0,nearest.z);
  EXPECT_EQ(1.0,on_edge.x);
  EXPECT_EQ(0.0,on_edge.y);
  EXPECT_EQ(0.0,on_edge.z);
}

TEST(pos_t,rot_xyz)
{
  TASCAR::pos_t px(1.0,0.0,0.0);
  TASCAR::pos_t py(0.0,1.0,0.0);
  TASCAR::pos_t pz(0.0,0.0,1.0);
  TASCAR::pos_t r;
  r = px;
  r.rot_z( TASCAR_PI2 );
  ASSERT_NEAR( 0.0, r.x, 1e-9 );
  ASSERT_NEAR( 1.0, r.y, 1e-9 );
  ASSERT_NEAR( 0.0, r.z, 1e-9 );
  r = px;
  r.rot_y( TASCAR_PI2 );
  ASSERT_NEAR( 0.0, r.x, 1e-9 );
  ASSERT_NEAR( 0.0, r.y, 1e-9 );
  ASSERT_NEAR( -1.0, r.z, 1e-9 );
  r = px;
  r.rot_x( TASCAR_PI2 );
  ASSERT_NEAR( 1.0, r.x, 1e-9 );
  ASSERT_NEAR( 0.0, r.y, 1e-9 );
  ASSERT_NEAR( 0.0, r.z, 1e-9 );
  //
  r = py;
  r.rot_z( TASCAR_PI2 );
  ASSERT_NEAR( -1.0, r.x, 1e-9 );
  ASSERT_NEAR( 0.0, r.y, 1e-9 );
  ASSERT_NEAR( 0.0, r.z, 1e-9 );
  r = py;
  r.rot_y( TASCAR_PI2 );
  ASSERT_NEAR( 0.0, r.x, 1e-9 );
  ASSERT_NEAR( 1.0, r.y, 1e-9 );
  ASSERT_NEAR( 0.0, r.z, 1e-9 );
  r = py;
  r.rot_x( TASCAR_PI2 );
  ASSERT_NEAR( 0.0, r.x, 1e-9 );
  ASSERT_NEAR( 0.0, r.y, 1e-9 );
  ASSERT_NEAR( 1.0, r.z, 1e-9 );
  //
  r = pz;
  r.rot_z( TASCAR_PI2 );
  ASSERT_NEAR( 0.0, r.x, 1e-9 );
  ASSERT_NEAR( 0.0, r.y, 1e-9 );
  ASSERT_NEAR( 1.0, r.z, 1e-9 );
  r = pz;
  r.rot_y( TASCAR_PI2 );
  ASSERT_NEAR( 1.0, r.x, 1e-9 );
  ASSERT_NEAR( 0.0, r.y, 1e-9 );
  ASSERT_NEAR( 0.0, r.z, 1e-9 );
  r = pz;
  r.rot_x( TASCAR_PI2 );
  ASSERT_NEAR( 0.0, r.x, 1e-9 );
  ASSERT_NEAR( -1.0, r.y, 1e-9 );
  ASSERT_NEAR( 0.0, r.z, 1e-9 );
}

TEST(pos_t,subdivide_mesh)
{
  std::vector<TASCAR::pos_t> mesh(TASCAR::generate_icosahedron());
  EXPECT_EQ( 12u, mesh.size() );
  mesh = TASCAR::subdivide_and_normalize_mesh( mesh, 1 );
  // one new vertex in every face, makes 12 old + 20 new vertices:
  EXPECT_EQ( 32u, mesh.size() );
  for( auto it=mesh.begin();it!=mesh.end();++it){
    ASSERT_NEAR( 1.0, it->norm(), 1e-9 );
  }
  mesh = TASCAR::generate_icosahedron();
  EXPECT_EQ( 12u, mesh.size() );
  mesh = TASCAR::subdivide_and_normalize_mesh( mesh, 2 );
  EXPECT_EQ( 92u, mesh.size() );
  mesh = TASCAR::subdivide_and_normalize_mesh( mesh, 1 );
  EXPECT_EQ( 272u, mesh.size() );
}

TEST(quaternion_t,rotate)
{
  TASCAR::quaternion_t q;
  q.set_rotation( TASCAR_PI2f, TASCAR::pos_t( 0, 0, 1 ) );
  ASSERT_NEAR( 1.0, q.norm(), 1e-7 );
  ASSERT_NEAR( cosf( 0.25f*TASCAR_PIf ), q.w, 1e-8f );
  ASSERT_NEAR( 0.0f, q.x, 1e-8 );
  ASSERT_NEAR( 0.0f, q.y, 1e-8 );
  ASSERT_NEAR( sinf( 0.25f*TASCAR_PIf ), q.z, 1e-8 );
  TASCAR::pos_t p( 0, 0, 1 );
  q.rotate( p );
  ASSERT_NEAR( 0.0, p.x, 1e-9 );
  ASSERT_NEAR( 0.0, p.y, 1e-9 );
  ASSERT_NEAR( 1.0, p.z, 1e-9 );
  p = TASCAR::pos_t( 1, 0, 0 );
  q.rotate( p );
  ASSERT_NEAR( 0.0, p.x, 1e-9 );
  ASSERT_NEAR( 1.0, p.y, 1e-9 );
  ASSERT_NEAR( 0.0, p.z, 1e-9 );
  q.set_rotation( TASCAR_PIf, TASCAR::pos_t( sqrt(0.5), 0, sqrt(0.5) ) );
  p = TASCAR::pos_t( 1, 0, 0 );
  q.rotate( p );
  ASSERT_NEAR( 0.0, p.x, 1e-9 );
  ASSERT_NEAR( 0.0, p.y, 1e-7 );
  ASSERT_NEAR( 1.0, p.z, 1e-9 );
  q.set_rotation( TASCAR_PI2f, TASCAR::pos_t( 0, 0, 1 ) );
  TASCAR::quaternion_t qp;
  qp.set_rotation( TASCAR_PI2f, TASCAR::pos_t( 0, 0, 1 ) );
  qp *= q;
  ASSERT_NEAR( cosf( TASCAR_PI2f ), qp.w, 1e-7 );
  ASSERT_NEAR( 0.0f, qp.x, 1e-8 );
  ASSERT_NEAR( 0.0f, qp.y, 1e-8 );
  ASSERT_NEAR( sinf( TASCAR_PI2f ), qp.z, 1e-7 );
  p = TASCAR::pos_t( 1, 0, 0 );
  qp.rotate( p );
  ASSERT_NEAR( -1.0, p.x, 1e-7 );
  ASSERT_NEAR( 0.0, p.y, 1e-7 );
  ASSERT_NEAR( 0.0, p.z, 1e-7 );
}

TEST(quaternion_t, euler)
{
  TASCAR::quaternion_t q;
  TASCAR::zyx_euler_t eul;
  // single axis rotation:
  for(float r = -1.5f; r <= 1.5f; r += 0.25f) {
    q.set_rotation(r, TASCAR::pos_t(0, 0, 1));
    eul = q.to_euler();
    ASSERT_NEAR(r, eul.z, 1e-7);
    ASSERT_NEAR(0.0, eul.y, 1e-9);
    ASSERT_NEAR(0.0, eul.x, 1e-9);
    q.set_rotation(r, TASCAR::pos_t(0, 1, 0));
    eul = q.to_euler();
    ASSERT_NEAR(0.0, eul.z, 1e-9);
    ASSERT_NEAR(r, eul.y, 1e-6);
    ASSERT_NEAR(0.0, eul.x, 1e-9);
    q.set_rotation(r, TASCAR::pos_t(1, 0, 0));
    eul = q.to_euler();
    ASSERT_NEAR(0.0, eul.z, 1e-9);
    ASSERT_NEAR(0.0, eul.y, 1e-9);
    ASSERT_NEAR(r, eul.x, 1e-7);
  }
  // multiple axis rotation:
  TASCAR::quaternion_t q2;
  q.set_rotation(0.2f, TASCAR::pos_t(0, 0, 1));
  q2.set_rotation(0.3f, TASCAR::pos_t(0, 1, 0));
  q *= q2;
  q2.set_rotation(0.4f, TASCAR::pos_t(1, 0, 0));
  q *= q2;
  eul = q.to_euler();
  ASSERT_NEAR(0.2f, eul.z, 1e-7);
  ASSERT_NEAR(0.3f, eul.y, 1e-7);
  ASSERT_NEAR(0.4f, eul.x, 1e-7);
  // to/from euler
  q.set_euler(TASCAR::zyx_euler_t(0.2, 0.3, 0.4));
  eul = q.to_euler();
  ASSERT_NEAR(0.2f, eul.z, 1e-7);
  ASSERT_NEAR(0.3f, eul.y, 1e-7);
  ASSERT_NEAR(0.4f, eul.x, 1e-7);
}

TEST(median, median)
{
  std::vector<float> data;
  // special return value for empty arrays:
  ASSERT_EQ(0.0, TASCAR::median(data.begin(), data.end()));
  data = {5.0f, 3.0f};
  // median of even number is mean of neighboring elements
  ASSERT_EQ(4.0, TASCAR::median(data.begin(), data.end()));
  // expected data to be partially sorted:
  ASSERT_EQ(3.0f, data[0]);
  data = {7.0f, 4.0f, 4.0f, 2.0f, 1.0f, 9.0f, 9.0f, 10.0f, 7.0f};
  ASSERT_EQ(7.0, TASCAR::median(data.begin(), data.end()));
}

TEST(pos, print)
{
  TASCAR::pos_t p(1.5, 0, 0);
  ASSERT_EQ("1.5, 0, 0", p.print_sphere());
  ASSERT_EQ("1.5, 0, 0", p.print_cart());
  TASCAR::pos_t p1(0, 1.5, 0);
  ASSERT_EQ("1.5, 90, 0", p1.print_sphere());
  ASSERT_EQ("0, 1.5, 0", p1.print_cart());
}

TEST(pos, euler)
{
  TASCAR::pos_t p(1, 0, 0);
  TASCAR::zyx_euler_t eul;
  eul.set_from_pos(p);
  ASSERT_NEAR(0.0, eul.z, 1e-7);
  ASSERT_NEAR(0.0, eul.y, 1e-7);
  ASSERT_NEAR(0.0, eul.x, 1e-7);
  p *= TASCAR::zyx_euler_t(0.3, 0.0, 0);
  eul.set_from_pos(p);
  ASSERT_NEAR(0.3, eul.z, 1e-7);
  ASSERT_NEAR(0.0, eul.y, 1e-7);
  ASSERT_NEAR(0.0, eul.x, 1e-7);
  p = TASCAR::pos_t(1, 0, 0);
  p *= TASCAR::zyx_euler_t(0.0, 0.2, 0);
  eul.set_from_pos(p);
  ASSERT_NEAR(0.0, eul.z, 1e-7);
  ASSERT_NEAR(0.2, eul.y, 1e-7);
  ASSERT_NEAR(0.0, eul.x, 1e-7);
  p = TASCAR::pos_t(1, 0, 0);
  p *= TASCAR::zyx_euler_t(0.3, 0.2, 0);
  eul.set_from_pos(p);
  ASSERT_NEAR(0.3, eul.z, 1e-7);
  ASSERT_NEAR(0.2, eul.y, 1e-7);
  ASSERT_NEAR(0.0, eul.x, 1e-7);
  p = TASCAR::pos_t(1, 0, 0);
  p *= TASCAR::zyx_euler_t(3.1, 0, 0);
  eul.set_from_pos(p);
  ASSERT_NEAR(0.041592653, eul.z, 1e-7);
  ASSERT_NEAR(TASCAR_PI, fabs(eul.y), 1e-7);
  ASSERT_NEAR(0.0, eul.x, 1e-7);
  p = TASCAR::pos_t(1, 0, 0);
  p *= TASCAR::zyx_euler_t(-0.6, 0, 0);
  eul.set_from_pos(p);
  ASSERT_NEAR(-0.6, eul.y, 1e-7);
  ASSERT_NEAR(0.0, eul.z, 1e-7);
  ASSERT_NEAR(0.0, eul.x, 1e-7);
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
