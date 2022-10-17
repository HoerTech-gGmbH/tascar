/**
 * @file   audioplugin.h
 * @author Giso Grimm
 *
 * @brief  Base class for audio signal processing plugins
 */
/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
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
#ifndef TICTOCTIMER_H
#define TICTOCTIMER_H

#include <sys/time.h>

namespace TASCAR {

  class tictoc_t {
  public:
    tictoc_t();
    void tic();
    double toc();
    double tictoc();

  private:
    struct timeval tv1;
    struct timeval tv2;
    struct timezone tz;
    double t;
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
