/**
   \file speakerlayout.cc
   \brief "dynpan" is a multichannel panning tool implementing several panning methods.
   \ingroup libtascar
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
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/
#include "speakerlayout.h"
#include <libxml++/libxml++.h>
#include <stdlib.h>
#include "defs.h"
#include <stdio.h>

TASCAR::speakerlayout_t TASCAR::loadspk(const std::string& fname)
{
  if( fname == "@ortf" )
    return TASCAR::spk_ortf( );
  if( fname == "@8" )
    return TASCAR::spk_ring( 8, 2.0, 1 );
  if( fname == "@12" )
    return TASCAR::spk_ring( 12, 2.0, 1 );
  if( fname == "@24" )
    return TASCAR::spk_ring( 24, 2.0, 1 );
  if( fname == "@64" )
    return TASCAR::spk_ring( 64, 2.5, 1 );
  if( fname == "@10" )
    return TASCAR::spk_ring( 10, 1.5, 13 );
  std::vector<TASCAR::pos_t> v_pos;
  std::vector<std::string> v_dest;
  // load XML file
  xmlpp::DomParser parser(fname);
  xmlpp::Element* root = parser.get_document()->get_root_node();
  if( root ){
    //xmlpp::Node::NodeList nodesSpk = root->get_children("Location");
    xmlpp::NodeSet nodesSpk = root->find("//speaker");
    for(xmlpp::NodeSet::iterator nit=nodesSpk.begin();nit!=nodesSpk.end();++nit){
      xmlpp::Element* loc = dynamic_cast<xmlpp::Element*>(*nit);
      if( loc ){
        std::string dest = loc->get_attribute_value("connect");
        TASCAR::pos_t pos;
        pos.set_sphere(atof(loc->get_attribute_value("dist").c_str()),
                       DEG2RAD*atof(loc->get_attribute_value("azim").c_str()),
                       DEG2RAD*atof(loc->get_attribute_value("elev").c_str()));
        v_pos.push_back(pos);
        v_dest.push_back(dest);
      }
    }
  }
  return TASCAR::speakerlayout_t(v_pos, v_dest);
}

/**
   \ingroup apptascar
*/
TASCAR::speakerlayout_t TASCAR::spk_ring(uint32_t n, double r, uint32_t first)
{
  std::vector<TASCAR::pos_t> v_pos;
  std::vector<std::string> v_dest;
  v_pos.resize(n);
  v_dest.resize(n);
  for(unsigned int k=0;k<n;k++){
    char ctmp[100];
    sprintf(ctmp,"system:playback_%d",k+first);
    v_pos[k].set_sphere( r, PI2*(double)k/n, 0.0 );
    v_dest[k] = ctmp;
  }
  return TASCAR::speakerlayout_t(v_pos, v_dest);
}

TASCAR::speakerlayout_t TASCAR::spk_ortf()
{
  std::vector<TASCAR::pos_t> v_pos;
  std::vector<std::string> v_dest;
  std::vector<std::string> v_name;
  v_name.push_back("ortf_l");
  v_name.push_back("ortf_r");
  v_pos.resize(2);
  v_dest.resize(2);
  for(unsigned int k=0;k<2;k++){
    char ctmp[100];
    sprintf(ctmp,"system:playback_%d",k+1);
    v_pos[k].set_sphere( 0.10377, -PI2*((double)k-0.5)*110/360, 0.0 );
    v_dest[k] = ctmp;
  }
  return TASCAR::speakerlayout_t(v_pos, v_dest, v_name);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
