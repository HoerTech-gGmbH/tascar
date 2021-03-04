#include "receivermod.h"
#include "amb33defs.h"
#include "errorhandling.h"

#include <limits>
#include "fft.h"
  
class foaconv_vars_t : public TASCAR::receivermod_base_t {
public:
  foaconv_vars_t( xmlpp::Element* xmlsrc );
  ~foaconv_vars_t();
protected:
  std::string irsname;
  uint32_t maxlen;
};

foaconv_vars_t::foaconv_vars_t( xmlpp::Element* xmlsrc )
  : receivermod_base_t( xmlsrc ),
    maxlen(0)
{
  GET_ATTRIBUTE(irsname,"","Name of IRS sound file");
  GET_ATTRIBUTE(maxlen,"samples","Maximum length of IRS, or 0 to use full sound file");
}

foaconv_vars_t::~foaconv_vars_t()
{
}

class foaconv_t : public foaconv_vars_t {
public:
  foaconv_t( xmlpp::Element* xmlsrc );
  ~foaconv_t();
  void postproc( std::vector<TASCAR::wave_t>& output );
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*){};
  void configure();
  receivermod_base_t::data_t* create_data(double srate,uint32_t fragsize);
private:
  TASCAR::sndfile_t irs1;
  TASCAR::sndfile_t irs2;
  TASCAR::sndfile_t irs3;
  TASCAR::sndfile_t irs4;
  std::vector<TASCAR::partitioned_conv_t*> conv;
  TASCAR::wave_t* rec_out;
  float wgain;
  bool is_acn;
};

foaconv_t::foaconv_t( xmlpp::Element* cfg )
  : foaconv_vars_t( cfg ),
    irs1( irsname, 0 ),
    irs2( irsname, 1 ),
    irs3( irsname, 2 ),
    irs4( irsname, 3 ),
    rec_out(NULL),
    wgain(MIN3DB),
    is_acn(false)
{
  std::string normalization("FuMa");
  GET_ATTRIBUTE(normalization,"","Normalization of FOA response, either ``FuMa'' or ``SN3D''");
  if( normalization == "FuMa" )
    wgain = MIN3DB;
  else if( normalization == "SN3D" )
    wgain = 1.0f;
  else throw TASCAR::ErrMsg("Currently, only FuMa and SN3D normalization is supported.");
  irs1 *= wgain;
  std::string channelorder("ACN");
  GET_ATTRIBUTE(channelorder,"","Channel order of FOA response, either ``FuMa'' (wxyz) or ``ACN'' (wyzx)");
  if( channelorder == "FuMa" )
    is_acn = false;
  else if( channelorder == "ACN" )
    is_acn = true;
  else throw TASCAR::ErrMsg("Currently, only FuMa and ACN channelorder is supported.");
}

void foaconv_t::configure( )
{
  receivermod_base_t::configure( );
  n_channels = AMB11::idx::channels;
  for( auto it=conv.begin();it!=conv.end();++it)
    delete *it;
  conv.clear();
  uint32_t irslen(irs1.get_frames());
  if( maxlen > 0 )
    irslen = std::min(maxlen,irslen);
  conv.push_back(new TASCAR::partitioned_conv_t( irslen, n_fragment ));
  conv.push_back(new TASCAR::partitioned_conv_t( irslen, n_fragment ));
  conv.push_back(new TASCAR::partitioned_conv_t( irslen, n_fragment ));
  conv.push_back(new TASCAR::partitioned_conv_t( irslen, n_fragment ));
  if( is_acn ){
    // wyzx
    conv[0]->set_irs( irs1 );
    conv[1]->set_irs( irs2 );
    conv[2]->set_irs( irs3 );
    conv[3]->set_irs( irs4 );
  }else{
    // wxyz
    conv[0]->set_irs( irs1 );
    conv[1]->set_irs( irs3 );
    conv[2]->set_irs( irs4 );
    conv[3]->set_irs( irs2 );
  }
  if( rec_out )
    delete rec_out;
  rec_out = new TASCAR::wave_t( n_fragment );
  labels.clear();
  for(uint32_t ch=0;ch<n_channels;++ch){
    char ctmp[32];
    sprintf(ctmp,".%d%c",(ch>0),AMB11::channelorder[ch]);
    labels.push_back(ctmp);
  }
}

foaconv_t::~foaconv_t()
{
  for( auto it=conv.begin();it!=conv.end();++it)
    delete *it;
  if( rec_out )
    delete rec_out;
}

void foaconv_t::postproc( std::vector<TASCAR::wave_t>& output )
{
  for( uint32_t c=0;c<4;++c)
    conv[c]->process( *rec_out, output[c], true );
  rec_out->clear();
}

TASCAR::receivermod_base_t::data_t* foaconv_t::create_data(double srate,uint32_t fragsize)
{
  return new data_t();
}

void foaconv_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  *rec_out += chunk;
}

REGISTER_RECEIVERMOD(foaconv_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

