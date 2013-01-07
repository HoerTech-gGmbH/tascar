#ifndef AMB33DEFS_H
#define AMB33DEFS_H

#define MIN3DB  0.707107f

namespace AMB33 {

  const char channelorder[] = "wyzxvtrsuqomklnp";

  class idx {
  public:
    enum {
      w, y, z, x, v, t, r, s, u, q, o, m, k, l, n, p, channels
    };
  };

};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
