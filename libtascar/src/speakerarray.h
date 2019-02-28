/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
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
#ifndef SPEAKERARRAY_H
#define SPEAKERARRAY_H

#include "xmlconfig.h"
#include "filterclass.h"
#include "delayline.h"
#include "ola.h"

namespace TASCAR {

  /**
     \brief Description of a single loudspeaker in a speaker array.
   */
  class spk_descriptor_t : public xml_element_t, public pos_t {
  public:
    // if you add members please update copy constructor!
    spk_descriptor_t(xmlpp::Element*);
    spk_descriptor_t(const spk_descriptor_t&);
    virtual ~spk_descriptor_t();
    double get_rel_azim(double az_src) const;
    double get_cos_adist(pos_t src_unit) const;
    double az;
    double el;
    double r;
    double delay;
    std::string label;
    std::string connect;
    //std::vector<double> compA;
    std::vector<double> compB;
    // derived parameters:
    pos_t unitvector;
    double gain;
    double dr;
    // decoder matrix:
    void update_foa_decoder(float gain, double xyzgain);
    float d_w;
    float d_x;
    float d_y;
    float d_z;
    float densityweight;
    TASCAR::overlap_save_t* comp;
  };

  /**
     \brief Loudspeaker array.
   */
  class spk_array_t : public xml_element_t, public std::vector<spk_descriptor_t>, public audiostates_t {
  public:
    spk_array_t(xmlpp::Element*, const std::string& elementname_="speaker");
    ~spk_array_t();
  private:
    spk_array_t(const spk_array_t&);
  public:
    double get_rmax() const { return rmax;};
    double get_rmin() const { return rmin;};
    class didx_t {
    public:
      didx_t() : d(0),idx(0) {};
      double d;
      uint32_t idx;
    };
    const std::vector<didx_t>& sort_distance(const pos_t& psrc);
    void render_diffuse(std::vector<TASCAR::wave_t>& output);
    void add_diffuse_sound_field( const TASCAR::amb1wave_t& diff );
    void prepare( chunk_cfg_t& );
  private:
    void import_file(const std::string& fname);
    void read_xml(xmlpp::Element* elem);
    TASCAR::amb1wave_t* diffuse_field_accumulator;
    TASCAR::wave_t* diffuse_render_buffer;
    double rmax;
    double rmin;
    std::vector<didx_t> didx;
    double xyzgain;
    std::string elementname;
    xmlpp::DomParser domp;
  public:
    std::vector<std::string> connections;
    std::vector<TASCAR::static_delay_t> delaycomp;
    double mean_rotation;
  private:
    std::vector<TASCAR::overlap_save_t> decorrflt;
  public:
    double decorr_length;
    bool decorr;
    bool densitycorr;
    double caliblevel;
    bool has_caliblevel;
    std::string layout;
    double calibage;
    std::string calibdate;
  };

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
