/**
 * @file   amb33defs.h
 * @author Giso Grimm
 * 
 * @brief  Ambisonics channel definitions up to 3rd order
 */
/* 
 * License (GPL)
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
#ifndef AMB33DEFS_H
#define AMB33DEFS_H

#define MIN3DB  0.707107f

namespace AMB11ACN {

  const char channelorder[] = "wyzx";

  class idx {
  public:
    enum { w, y, z, x, channels };
  };

}; // namespace AMB11ACN

namespace AMB11FuMa {

  const char channelorder[] = "wxyz";

  class idx {
  public:
    enum { w, x, y, z, channels };
  };

}; // namespace AMB11FuMa

namespace AMB10 {

  const char channelorder[] = "wyx";

  class idx {
  public:
    enum { w, y, x, channels };
  };

}; // namespace AMB10

namespace AMB33 {

  const char channelorder[] = "wyzxvtrsuqomklnp";

  class idx {
  public:
    enum { w, y, z, x, v, t, r, s, u, q, o, m, k, l, n, p, channels };
  };

}; // namespace AMB33

namespace AMB30 {

  const char channelorder[] = "wyxvuqp";

  class idx {
  public:
    enum { w, y, x, v, u, q, p, channels };
  };

}; // namespace AMB30

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
