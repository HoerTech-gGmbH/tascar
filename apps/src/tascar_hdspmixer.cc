/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

/*
 * hos_mainmix - read gain from jack port, create direct mapping in
 * HDSP 9652 from software output to hardware output with gain from
 * jack port.
 */

#include "cli.h"
#include "tascar.h"
#include <alsa/asoundlib.h>
#include <getopt.h>
#include <iostream>

class mainmix_t {
public:
  mainmix_t(const std::string& dev, uint32_t channels);
  virtual ~mainmix_t();
  void activate(uint32_t ch_in, uint32_t ch_out, uint32_t nch);

private:
  snd_ctl_elem_id_t* id;
  snd_ctl_elem_value_t* ctl;
  snd_ctl_t* handle;
};

mainmix_t::mainmix_t(const std::string& device, uint32_t channels)
{
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_id_set_name(id, "Mixer");
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_HWDEP);
  snd_ctl_elem_id_set_device(id, 0);
  snd_ctl_elem_id_set_index(id, 0);
  snd_ctl_elem_value_alloca(&ctl);
  snd_ctl_elem_value_set_id(ctl, id);
  int err;
  if((err = snd_ctl_open(&handle, device.c_str(), SND_CTL_NONBLOCK)) < 0) {
    std::string msg("Alsa error: ");
    msg += snd_strerror(err);
    msg += " (device: " + device + ")";
    throw TASCAR::ErrMsg(msg);
  }
  for(uint32_t k_in = 0; k_in < 2 * channels; k_in++) {
    for(uint32_t k_out = 0; k_out < channels; k_out++) {
      snd_ctl_elem_value_set_integer(ctl, 0, k_in);
      snd_ctl_elem_value_set_integer(ctl, 1, k_out);
      snd_ctl_elem_value_set_integer(ctl, 2, 0);
      if((err = snd_ctl_elem_write(handle, ctl)) < 0) {
        std::string msg("Alsa error: ");
        msg += snd_strerror(err);
        throw TASCAR::ErrMsg(msg);
      }
    }
  }
}

mainmix_t::~mainmix_t()
{
  snd_ctl_close(handle);
}

void mainmix_t::activate(uint32_t ch_in, uint32_t ch_out, uint32_t nch)
{
  for(uint32_t k = 0; k < nch; k++) {
    snd_ctl_elem_value_set_integer(ctl, 0, k + ch_in);
    snd_ctl_elem_value_set_integer(ctl, 1, k + ch_out);
    snd_ctl_elem_value_set_integer(ctl, 2, 32767);
    int err(0);
    if((err = snd_ctl_elem_write(handle, ctl)) < 0) {
      std::string msg("Alsa error: ");
      msg += snd_strerror(err);
      throw TASCAR::ErrMsg(msg);
    }
  }
}

// void usage(struct option * opt)
//{
//   std::cout << "Usage:\n\ntascar_hdspmixer [options]\n\nOptions:\n\n";
//   std::cout << "Simple control of HDSP 9652 matrix mixer\n";
//   while( opt->name ){
//     std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
//       "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
//     opt++;
//   }
// }

int main(int argc, char** argv)
{
  const char* options = "hisad:c:";
  struct option long_options[] = {
      {"help", 0, 0, 'h'},   {"input", 0, 0, 'i'},  {"alsa", 0, 0, 'a'},
      {"stereo", 0, 0, 's'}, {"device", 1, 0, 'd'}, {"channels", 1, 0, 'c'},
      {0, 0, 0, 0}};
  std::string device("hw:DSP");
  uint32_t channels(26);
  bool b_input(false);
  bool b_soft(false);
  bool b_stereo(false);
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      TASCAR::app_usage("tascar_hdspmixer", long_options, "",
                        "Simple control of HDSP 9652 matrix mixer.\n\n");
      // usage(long_options);
      return 0;
    case 'i':
      b_input = true;
      break;
    case 'a':
      b_soft = true;
      break;
    case 's':
      b_stereo = true;
      break;
    case 'd':
      device = optarg;
      break;
    case 'c':
      channels = atoi(optarg);
      break;
    }
  }
  try {
    mainmix_t mm(device, channels);
    if(b_input)
      mm.activate(0, 0, channels);
    if(b_soft)
      mm.activate(channels, 0, channels);
    if(b_stereo) {
      for(uint32_t k = 0; k < (channels >> 1); k++) {
        mm.activate(channels, 2 * k, 2);
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
