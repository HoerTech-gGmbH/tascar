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
#ifndef IRRENDER_H
#define IRRENDER_H

#include <time.h>

#include "audiochunks.h"
#include "session.h"

namespace TASCAR {

  /**
     \brief offline rendering of scenes

     This class can render scenes and impulse responses of the image
     source model part of a virtual scene.
   */
  class wav_render_t : public TASCAR::session_core_t {
  public:
    wav_render_t( const std::string& tscname, const std::string& scene,
                  bool verbose = false );
    void set_channelmap( const std::vector<size_t>& channels );
    void reset_channelmap();
    void set_ism_order_range( uint32_t ism_min, uint32_t ism_max );
    void render( uint32_t fragsize, const std::string& ifname,
                 const std::string& ofname, double starttime, bool b_dynamic );
    void render( uint32_t fragsize, double srate, double duration,
                 const std::string& ofname, double starttime, bool b_dynamic );
    void render_ir( uint32_t len, double fs, const std::string& ofname,
                    double starttime, uint32_t inputchannel );
    ~wav_render_t();
    virtual void validate_attributes(std::string&) const;

  protected:
    void add_scene( tsccfg::node_t e );
    std::string scene;
    render_core_t* pscene;
    bool verbose_;
    std::vector<size_t> ochannels;

  public:
    clock_t t0;
    clock_t t1;
    clock_t t2;
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
