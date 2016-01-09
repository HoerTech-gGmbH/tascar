#ifndef AMB33DEFS_H
#define AMB33DEFS_H

#define MIN3DB  0.707107f

namespace AMB11 {

  const char channelorder[] = "wyzx";

  class idx {
  public:
    enum {
      w, y, z, x, channels
    };
  };

};

namespace AMB10 {

  const char channelorder[] = "wyx";

  class idx {
  public:
    enum {
      w, y, x, channels
    };
  };

};

namespace AMB33 {

  const char channelorder[] = "wyzxvtrsuqomklnp";

  class idx {
  public:
    enum {
      w, y, z, x, v, t, r, s, u, q, o, m, k, l, n, p, channels
    };
  };

};

namespace AMB30 {

  const char channelorder[] = "wyxvuqp";

  class idx {
  public:
    enum {
      w, y, x, v, u, q, p, channels
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
