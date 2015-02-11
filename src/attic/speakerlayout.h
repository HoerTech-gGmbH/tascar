/**
   \file speakerlayout.h
   \ingroup libtascar
   \brief "dynpan" is a multichannel panning tool implementing several panning methods.
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
   General Lesser Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/
#ifndef SPEAKERLAYOUT_H
#define SPEAKERLAYOUT_H

#include "coordinates.h"
#include <string>
#include <vector>
#include <stdint.h>

namespace TASCAR {

  /**
     \ingroup libtascar
  */
  class speakerlayout_t {
  public:
    speakerlayout_t(const std::vector<TASCAR::pos_t>& pspk,const std::vector<std::string>& dest,const std::vector<std::string>& name = std::vector<std::string>());
    void print();
    uint32_t n;
    std::vector<pos_t> p;
    std::vector<pos_t> p_norm;
    std::vector<double> d;
    std::vector<double> az;
    double d_max;
    double d_min;
    const std::string& get_dest(unsigned int k) { return dest_[k]; };
    const std::string& get_name(unsigned int k) { return name_[k]; };
  private:
    std::vector<std::string> dest_;
    std::vector<std::string> name_;
  };

  /**
     \ingroup libtascar
     \brief Load a speaker layout from an XML file
  */
  TASCAR::speakerlayout_t loadspk(const std::string& fname);

  /**
     \ingroup libtascar
     \brief Create an ORTF (microphone) layout
  */
  TASCAR::speakerlayout_t spk_ortf();

  /**
     \ingroup libtascar
     \brief Create a regular loudspeaker ring
     \param n number of speakers
     \param r radius of speaker ring
     \param first first number of hardware output port
     \retval speaker layout
  */
  TASCAR::speakerlayout_t spk_ring(uint32_t n, double r, uint32_t first);

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
