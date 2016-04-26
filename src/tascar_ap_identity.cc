#include "audioplugin.h"

class identity_t : public TASCAR::audioplugin_base_t {
public:
  identity_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname) : audioplugin_base_t(xmlsrc,name,parentname) {};
  void process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos)  {};
};

REGISTER_AUDIOPLUGIN(identity_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
