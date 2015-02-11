/**
   \file tascar_dec_h Horizontal Ambisonics Decoder for any order

*/

#include <iostream>
#include "tascar.h"
#include "speakerlayout.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <gsl/gsl_sf.h>

class amb_mixed_order_chunk_t {
public:
  amb_mixed_order_chunk_t( unsigned int order_h, unsigned int order_v, unsigned int frames );
  ~amb_mixed_order_chunk_t();
  unsigned int num_channels() const {return n_total;};
  unsigned int num_frames() const {return frames_;};
  float Y(unsigned int ch,float azim, float elev);
protected:
  unsigned int order_h_;
  unsigned int order_v_;
  unsigned int n_full;
  unsigned int n_h_full;
  unsigned int n_h;
  unsigned int n_h_only;
  unsigned int n_total;
  unsigned int frames_;
  float* data;
  uint32_t* vl; // degree, see http://ambisonics.ch/
  uint32_t* vm; // order, see http://ambisonics.ch/

  //ACN = l * ( l + 1 ) + m
};

amb_mixed_order_chunk_t::amb_mixed_order_chunk_t( unsigned int order_h, unsigned int order_v, unsigned int frames )
  : order_h_(order_h),
    order_v_(order_v),
    n_full((order_v+1)*(order_v+1)),
    n_h_full(2*order_v+1),
    n_h(2*order_h+1),
    n_h_only(n_h-n_h_full),
    n_total(n_full+n_h_only),
    frames_(frames),
    data(NULL)
{
  if( order_h < order_v )
    throw TASCAR::ErrMsg("Horizontal order need to be at least the same as vertical order");
  data = new float[n_total*frames];
  vl = new uint32_t[n_total];
  vm = new uint32_t[n_total];
  int k=0;
  for( int l=0;l<=(int)order_v;l++){
    for( int m=-l;m<=l;m++ ){
      vl[k] = l;
      vm[k] = m;
      k++;
    }
  }
  for( int l=order_v+1;l<=(int)order_h;l++){
    vl[k] = l;
    vm[k] = -l;
    k++;
    vl[k] = l;
    vm[k] = l;
    k++;
  }
  for( unsigned int k=0;k<n_total;k++){
    printf("%d (%d,%d)\n",k,vl[k],vm[k]);
  }
  DEBUG(order_h);
  DEBUG(order_v);
  DEBUG(n_full);
  DEBUG(n_h_only);
  DEBUG(n_total);
}

amb_mixed_order_chunk_t::~amb_mixed_order_chunk_t()
{
  delete [] data;
}

float amb_mixed_order_chunk_t::Y(unsigned int ch,float azim, float elev)
{
  // this is wrong, please correct!
  return gsl_sf_legendre_sphPlm( vl[ch], vm[ch], azim );
}


class decoder_matrix_t {
public:
  decoder_matrix_t( unsigned int order_h, unsigned int order_v, TASCAR::speakerlayout_t spk );
private:
  TASCAR::speakerlayout_t spk_;
  amb_mixed_order_chunk_t decm;
};

decoder_matrix_t::decoder_matrix_t( unsigned int order_h, unsigned int order_v, TASCAR::speakerlayout_t spk )
  : spk_(spk),
    decm(order_h,order_v,spk.n)
{
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_ambdecoder speakerlayout [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  try{
    unsigned int order_h(3);
    unsigned int order_v(0);
    std::string speakerlayout("@8");
    std::string jackname("tascar_ambdecoder");
    const char *options = "o:v:hn:";
    struct option long_options[] = { 
      { "horiz",    1, 0, 'o' },
      { "vert",     1, 0, 'v' },
      { "speaker",  1, 0, 's' },
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'n' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 's':
        speakerlayout = optarg;
        break;
      case 'h':
        usage(long_options);
        return -1;
      case 'n':
        jackname = optarg;
        break;
      case 'o':
        order_h = atoi(optarg);
        break;
      case 'v':
        order_v = atoi(optarg);
        break;
      }
    }
    TASCAR::speakerlayout_t spk(TASCAR::loadspk(speakerlayout));;
    decoder_matrix_t ch(order_h,order_v,spk);
  }
  catch( const std::exception& e ){
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
