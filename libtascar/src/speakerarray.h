#ifndef SPEAKERARRAY_H
#define SPEAKERARRAY_H

#include "xmlconfig.h"
#include "filterclass.h"
#include "delayline.h"
//#include "audiostates.h"

namespace TASCAR {

    class spk_pos_t : public xml_element_t, public pos_t {
    public:
      // if you add members please update copy constructor!
      spk_pos_t(xmlpp::Element*);
      spk_pos_t(const spk_pos_t&);
      virtual ~spk_pos_t();
      double get_rel_azim(double az_src) const;
      double get_cos_adist(pos_t src_unit) const;
      double az;
      double el;
      double r;
      std::string label;
      std::string connect;
      std::vector<double> compA;
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
      TASCAR::filter_t* comp;
    };

  class spk_array_t : public xml_element_t, public std::vector<spk_pos_t>, public audiostates_t {
    public:
      spk_array_t(xmlpp::Element*, const std::string& elementname_="speaker");
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
      void foa_decode(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output);
      void prepare( chunk_cfg_t& );
    private:
      void import_file(const std::string& fname);
      void read_xml(xmlpp::Element* elem);
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
