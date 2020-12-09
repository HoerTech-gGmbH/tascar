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

#include "delayline.h"
#include "filterclass.h"
#include "ola.h"
#include "xmlconfig.h"

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
    std::vector<double> compB;
    double gain;
    // derived parameters:
    pos_t unitvector;
    double spkgain;
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

  class spk_array_cfg_t : public xml_element_t {
  public:
    spk_array_cfg_t(xmlpp::Element*, bool use_parent_xml);
    std::string layout;
    std::string name;

  protected:
    xmlpp::DomParser domp;
    xmlpp::Element* e_layout;
  };

  /**
     \brief Loudspeaker array.
   */
  class spk_array_t : public spk_array_cfg_t,
                      public std::vector<spk_descriptor_t>,
                      public audiostates_t {
  public:
    spk_array_t(xmlpp::Element*, bool use_parent_xml,
                const std::string& elementname_ = "speaker");
    ~spk_array_t();

  private:
    spk_array_t(const spk_array_t&);

  public:
    double get_rmax() const { return rmax; };
    double get_rmin() const { return rmin; };
    class didx_t {
    public:
      didx_t() : d(0), idx(0){};
      double d;
      uint32_t idx;
    };
    const std::vector<didx_t>& sort_distance(const pos_t& psrc);
    void configure();
    xml_element_t elayout;
    void validate_attributes(std::string& msg) const;
    std::vector<TASCAR::pos_t> get_positions() const;

  private:
    double rmax;
    double rmin;
    double xyzgain;
    std::string onload;
    std::string onunload;
    std::vector<didx_t> didx;
    std::string elementname;

  public:
    std::vector<std::string> connections;
    std::vector<TASCAR::static_delay_t> delaycomp;
    double mean_rotation;
  };

  class spk_array_diff_render_t : public spk_array_t {
  public:
    spk_array_diff_render_t(xmlpp::Element*, bool use_parent_xml,
                            const std::string& elementname_ = "speaker");
    ~spk_array_diff_render_t();
    void render_diffuse(std::vector<TASCAR::wave_t>& output);
    void add_diffuse_sound_field(const TASCAR::amb1wave_t& diff);
    void configure();

  private:
    void read_xml(xmlpp::Element* elem);
    TASCAR::amb1wave_t* diffuse_field_accumulator;
    TASCAR::wave_t* diffuse_render_buffer;
    std::vector<TASCAR::overlap_save_t> decorrflt;

  public:
    double decorr_length;
    bool decorr;
    bool densitycorr;
    double caliblevel;
    bool has_caliblevel;
    double diffusegain;
    bool has_diffusegain;
    double calibage;
    std::string calibdate;
    bool has_calibdate;
    std::string calibfor;
    bool has_calibfor;
  };
} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
