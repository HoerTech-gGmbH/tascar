#include "sourcemod.h"

class omni_t : public TASCAR::sourcemod_base_t {
public:
  omni_t(tsccfg::node_t xmlsrc);
};

omni_t::omni_t(tsccfg::node_t xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc)
{
}

REGISTER_SOURCEMOD(omni_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
