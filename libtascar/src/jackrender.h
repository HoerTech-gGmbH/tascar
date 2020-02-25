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
#ifndef JACKRENDER_H
#define JACKRENDER_H

#include "render.h"
#include "osc_scene.h"

namespace TASCAR {

  class scene_container_t {
  public:
    scene_container_t(const std::string& xmlfile);
    scene_container_t(TASCAR::Scene::scene_t* scenesrc);
    ~scene_container_t();
  protected:
    xmlpp::Document* xmldoc;
    TASCAR::Scene::scene_t* scene;
    bool own_pointer;
  };    
  
  class scene_render_rt_t : public TASCAR::render_core_t, public TASCAR::Scene::osc_scene_t, public jackc_transport_t  {
  public:
    scene_render_rt_t(xmlpp::Element* xmlsrc);
    virtual ~scene_render_rt_t();
    void run(bool &b_quit);
    void start();
    void stop();
  private:
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
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
