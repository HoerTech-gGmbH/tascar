/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2023 Giso Grimm
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

#include "pdfexport.h"
#include "session.h"

class sendsvg_t : public TASCAR::module_base_t {
public:
  sendsvg_t(const TASCAR::module_cfg_t& cfg);
  virtual ~sendsvg_t(){};

private:
};

int osc_sendsvg(const char*, const char* types, lo_arg** argv, int argc,
                lo_message, void* user_data)
{
  TASCAR::session_t* h((TASCAR::session_t*)user_data);
  if(h && (argc == 4) && (types[0] == 's') && (types[1] == 's') &&
     (types[2] == 'f') && (types[3] == 'f')) {
    lo_address target = lo_address_new_from_url(&(argv[0]->s));
    if(target) {
      lo_send(target, &(argv[1]->s), "s",
              TASCAR::export_svg(h, argv[2]->f, argv[3]->f).c_str());
      lo_address_free(target);
    }
    return 0;
  }
  return 1;
}

sendsvg_t::sendsvg_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)
{
  session->add_method("/sendsvg", "ssff", &osc_sendsvg, session);
}

REGISTER_MODULE(sendsvg_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
