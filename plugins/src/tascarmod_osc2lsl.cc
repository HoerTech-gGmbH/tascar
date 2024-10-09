/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2024 Giso Grimm
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

#include "session.h"
#include <lsl_cpp.h>

class osc2lsl_t : public TASCAR::module_base_t {
public:
  osc2lsl_t(const TASCAR::module_cfg_t& cfg);
  ~osc2lsl_t();
  static int osc_recv(const char* path, const char* types, lo_arg** argv,
                      int argc, lo_message msg, void* user_data);
  int osc_recv(lo_arg** argv, int argc);

private:
  std::string path = "/osc2lsl";
  uint32_t size = 1u;
  bool first_row_is_timestamp = false;
  std::string lslname = "osc2lsl";
  std::string lsltype = "osc2lsl";
  std::string source_id = "osc2lsl" + TASCAR::get_tuid();
  int retval = 1;
  lsl::stream_outlet* sop = NULL;
  std::vector<double> data;
};

int osc2lsl_t::osc_recv(const char*, const char*, lo_arg** argv, int argc,
                        lo_message, void* user_data)
{
  return ((osc2lsl_t*)user_data)->osc_recv(argv, argc);
}

int osc2lsl_t::osc_recv(lo_arg** argv, int argc)
{
  if(argc != (int)size)
    return retval;
  std::uint32_t startchannel(first_row_is_timestamp);
  for(std::uint32_t k = startchannel; k < size; ++k)
    data[k] = argv[k]->d;
  if(first_row_is_timestamp)
    sop->push_sample(data, argv[0]->d);
  else
    sop->push_sample(data);
  return retval;
}

osc2lsl_t::osc2lsl_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)
{
  GET_ATTRIBUTE(path, "", "OSC path name");
  GET_ATTRIBUTE(size, "", "Dimension of variable");
  GET_ATTRIBUTE_BOOL(first_row_is_timestamp,
                     "Use data of first row as LSL time stamp");
  GET_ATTRIBUTE(lslname, "", "LSL name");
  GET_ATTRIBUTE(lsltype, "", "LSL type");
  GET_ATTRIBUTE(source_id, "", "LSL source ID");
  GET_ATTRIBUTE(retval, "",
                "OSC return value: 0 = handle messages also locally, non-0 = "
                "mark message as handled, do not handle locally");
  int numchannels(size);
  if(first_row_is_timestamp && (size > 1))
    --numchannels;
  lsl::stream_info info(lslname, lsltype, numchannels, lsl::IRREGULAR_RATE,
                        lsl::cf_double64, source_id);
  sop = new lsl::stream_outlet(info);
  data.resize(numchannels);
  session->add_method(path, std::string(size, 'd').c_str(),
                      &osc2lsl_t::osc_recv, this);
}

osc2lsl_t::~osc2lsl_t()
{
  // lo_address_free(target);
  delete sop;
}

REGISTER_MODULE(osc2lsl_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
