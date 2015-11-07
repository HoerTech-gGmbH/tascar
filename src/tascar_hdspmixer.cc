
/*
 * hos_mainmix - read gain from jack port, create direct mapping in
 * HDSP 9652 from software output to hardware output with gain from
 * jack port.
 */

#include <alsa/asoundlib.h>
#include <getopt.h>
#include <iostream>
#include "tascar.h"

class mainmix_t {
public:
  mainmix_t();
  virtual ~mainmix_t();
  void activate(uint32_t ch_in, uint32_t ch_out,uint32_t nch);
private:
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *ctl;
  snd_ctl_t *handle;
};

mainmix_t::mainmix_t()
{
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_id_set_name(id, "Mixer");
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_HWDEP);
  snd_ctl_elem_id_set_device(id, 0);
  snd_ctl_elem_id_set_index(id, 0);
  snd_ctl_elem_value_alloca(&ctl);
  snd_ctl_elem_value_set_id(ctl, id);
  int err;
  if ((err = snd_ctl_open(&handle, "hw:DSP", SND_CTL_NONBLOCK)) < 0) {
    std::string msg("Alsa error: ");
    msg += snd_strerror(err);
    throw TASCAR::ErrMsg(msg);
  }
  for( uint32_t k_in=0;k_in<52;k_in++ ){
    for( uint32_t k_out=0;k_out<26;k_out++ ){
      snd_ctl_elem_value_set_integer(ctl, 0, k_in);
      snd_ctl_elem_value_set_integer(ctl, 1, k_out);
      snd_ctl_elem_value_set_integer(ctl, 2, 0);
      if ((err = snd_ctl_elem_write(handle, ctl)) < 0) {
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

void mainmix_t::activate(uint32_t ch_in, uint32_t ch_out,uint32_t nch)
{
  for( uint32_t k=0;k<nch;k++ ){
    snd_ctl_elem_value_set_integer(ctl, 0, k+ch_in);
    snd_ctl_elem_value_set_integer(ctl, 1, k+ch_out);
    snd_ctl_elem_value_set_integer(ctl, 2, 32767);
    int err(0);
    if ((err = snd_ctl_elem_write(handle, ctl)) < 0) {
      std::string msg("Alsa error: ");
      msg += snd_strerror(err);
      throw TASCAR::ErrMsg(msg);
    }
  }
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_hdspmixer [options]\n\nOptions:\n\n";
  std::cout << "Simple control of HDSP 9652 matrix mixer\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  const char *options = "hisa";
  struct option long_options[] = { 
    { "help",      0, 0, 'h' },
    { "input",      0, 0, 'i' },
    { "alsa",      0, 0, 'a' },
    { "stereo",      0, 0, 's' },
    { 0, 0, 0, 0 }
  };
  bool b_input(false);
  bool b_soft(false);
  bool b_stereo(false);
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'h':
      usage(long_options);
      return -1;
    case 'i':
      b_input = true;
      break;
    case 'a':
      b_soft = true;
      break;
    case 's':
      b_stereo = true;
      break;
    }
  }
  try{
    mainmix_t mm;
    if( b_input )
      mm.activate(0,0,26);
    if( b_soft )
      mm.activate(26,0,26);
    if( b_stereo ){
      for(uint32_t k=0;k<13;k++){
        mm.activate(26+2*k,24,2);
      }
    }
  }
  catch( const std::exception& e ){
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
