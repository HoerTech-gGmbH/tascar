/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "maskplugin.h"

using namespace TASCAR;

class multibeam_t : public maskplugin_base_t {
public:
  multibeam_t(const maskplugin_cfg_t& cfg);
  float get_gain(const pos_t& pos);
  void get_diff_gain(float& gw, float& gy, float& gz, float& gx);
  void add_variables(TASCAR::osc_server_t* srv);

private:
  void update_steer();
  void resize_val();
  size_t numbeams = 1u;
  float mingain = 0.0f;
  float maxgain = 1.0f;

  std::vector<pos_t> vsteer;

public:
  std::vector<float> gain;
  std::vector<float> az;
  std::vector<float> el;
  std::vector<float> selectivity;
};

void resize_with_default(std::vector<float>& vec, float def, size_t n)
{
  for(size_t k = vec.size(); k < n; ++k)
    vec.push_back(def);
  vec.resize(n);
}

void multibeam_t::resize_val()
{
  resize_with_default(gain, 1.0f, numbeams);
  resize_with_default(az, 0.0f, numbeams);
  resize_with_default(el, 0.0f, numbeams);
  resize_with_default(selectivity, 90.0f, numbeams);
}

multibeam_t::multibeam_t(const maskplugin_cfg_t& cfg) : maskplugin_base_t(cfg)
{
  GET_ATTRIBUTE(numbeams, "", "Number of beams");
  resize_val();
  GET_ATTRIBUTE_DB(mingain, "Minimum gain");
  GET_ATTRIBUTE_DB(maxgain, "Maximum gain");
  GET_ATTRIBUTE_DB(gain, "On-axis gain");
  GET_ATTRIBUTE(az, "deg", "Azimuth of steering vectors");
  GET_ATTRIBUTE(el, "deg", "Elevation of steering vectors");
  GET_ATTRIBUTE(selectivity, "1/pi",
                "Selectivity, 0 = omni, 1 = cardioid (6 dB threshold)");
  resize_val();
  vsteer.resize(numbeams);
  update_steer();
}

void multibeam_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->add_vector_float_db("/gain", &gain);
  srv->add_vector_float("/selectivity", &selectivity);
  srv->add_vector_float("/az", &az);
  srv->add_vector_float("/el", &el);
  srv->add_float_db("/mingain", &mingain);
  srv->add_float_db("/maxgain", &maxgain);
}

void multibeam_t::update_steer()
{
  for(size_t k = 0u; k < numbeams; ++k)
    vsteer[k].set_sphere(1.0, DEG2RAD * az[k], DEG2RAD * el[k]);
}

float multibeam_t::get_gain(const pos_t& pos)
{
  update_steer();
  TASCAR::pos_t rp(pos.normal());
  float pgain = 0.0f;
  for(size_t k = 0; k < numbeams; ++k) {
    float ang =
        std::min(selectivity[k] * acosf(dot_prod(rp, vsteer[k])), TASCAR_PIf);
    pgain += gain[k] * (0.5f + 0.5f * cosf(ang));
  }
  pgain = std::min(maxgain, mingain + (1.0f - mingain) * pgain);
  return pgain;
};

void multibeam_t::get_diff_gain(float& gw, float& gy, float& gz, float& gx)
{
  float pgainw = 0.0f;
  float pgainy = 0.0f;
  float pgainz = 0.0f;
  float pgainx = 0.0f;
  for(size_t k = 0; k < numbeams; ++k) {
    pgainw += gain[k];
    float zyxw = gain[k] * (1.0f - std::min(selectivity[k], 1.0f));
    pgainy += zyxw * vsteer[k].y;
    pgainz += zyxw * vsteer[k].z;
    pgainx += zyxw * vsteer[k].x;
  }
  gw *= std::min(maxgain, mingain + (1.0f - mingain) * pgainw);
  gy *= std::min(maxgain, mingain + (1.0f - mingain) * pgainy);
  gz *= std::min(maxgain, mingain + (1.0f - mingain) * pgainz);
  gx *= std::min(maxgain, mingain + (1.0f - mingain) * pgainx);
}

REGISTER_MASKPLUGIN(multibeam_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
