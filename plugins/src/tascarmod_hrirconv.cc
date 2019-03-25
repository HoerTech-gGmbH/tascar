#include "jackclient.h"
#include "session.h"
#include <string.h>
#include "ola.h"

class channel_entry_t {
public:
  uint32_t inchannel;
  uint32_t outchannel;
  std::string filename;
  uint32_t filechannel;
};

class hrirconv_t : public jackc_t {
public:
  hrirconv_t(uint32_t inchannels,uint32_t outchannels,uint32_t fftlen, const std::vector<channel_entry_t>& matrix,const std::string& jackname, TASCAR::xml_element_t& e);
  virtual ~hrirconv_t();
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
private:
  std::vector<TASCAR::overlap_save_t*> cnv;
  std::vector<channel_entry_t> matrix_;
};

hrirconv_t::hrirconv_t(uint32_t inchannels,uint32_t outchannels,uint32_t fftlen, const std::vector<channel_entry_t>& matrix,const std::string& jackname, TASCAR::xml_element_t& e)
  : jackc_t(jackname),matrix_(matrix)
{
  if( fftlen <= (uint32_t)get_fragsize() ){
    DEBUG(fftlen);
    DEBUG(get_fragsize());
    throw TASCAR::ErrMsg("Invalid fft length.");
  }
  for(std::vector<channel_entry_t>::iterator mit=matrix_.begin();mit!=matrix_.end();++mit){
    if( mit->inchannel >= inchannels ){
      DEBUG(mit->inchannel);
      DEBUG(inchannels);
      throw TASCAR::ErrMsg("Invalid input channel number.");
    }
    if( mit->outchannel >= outchannels ){
      DEBUG(mit->outchannel);
      DEBUG(outchannels);
      throw TASCAR::ErrMsg("Invalid output channel number.");
    }
  }
  for(std::vector<channel_entry_t>::iterator mit=matrix_.begin();mit!=matrix_.end();++mit){
    TASCAR::sndfile_t sndf(mit->filename,mit->filechannel);
    if( sndf.get_srate() != (uint32_t)get_srate() ){
      std::ostringstream msg;
      msg << "Warning: The sample rate of file \"" << mit->filename << "\" (" << sndf.get_srate() << ") differs from system sample rate (" << get_srate() << ").";
      TASCAR::add_warning(msg.str(),e.e);
    }
    TASCAR::overlap_save_t* pcnv(new TASCAR::overlap_save_t(fftlen-fragsize+1,get_fragsize()));
    pcnv->set_irs(sndf,false);
    cnv.push_back(pcnv);
  }
  for( uint32_t k=0;k<inchannels;k++){
    char ctmp[256];
    sprintf(ctmp,"in_%d",k);
    add_input_port(ctmp);
  }
  for( uint32_t k=0;k<outchannels;k++){
    char ctmp[256];
    sprintf(ctmp,"out_%d",k);
    add_output_port(ctmp);
  }
  activate();
}

hrirconv_t::~hrirconv_t()
{
  deactivate();
  for(std::vector<TASCAR::overlap_save_t*>::iterator it=cnv.begin();it!=cnv.end();++it)
    delete *it;
}


int hrirconv_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  uint32_t Nout(outBuffer.size());
  for(uint32_t kOut=0;kOut<Nout;kOut++)
    memset(outBuffer[kOut],0,sizeof(float)*nframes);
  for(uint32_t k=0;k<cnv.size();k++){
    if( matrix_[k].inchannel >= inBuffer.size()){
      DEBUG(k);
      DEBUG(matrix_[k].inchannel);
      DEBUG(inBuffer.size());
    }
    if( matrix_[k].outchannel >= outBuffer.size()){
      DEBUG(k);
      DEBUG(matrix_[k].outchannel);
      DEBUG(outBuffer.size());
    }
    TASCAR::wave_t w_in(nframes,inBuffer[matrix_[k].inchannel]);
    TASCAR::wave_t w_out(nframes,outBuffer[matrix_[k].outchannel]);
    cnv[k]->process(w_in,w_out);
  }
  return 0;
}

class hrirconv_var_t : public TASCAR::module_base_t {
public:
  hrirconv_var_t( const TASCAR::module_cfg_t& cfg );
  std::string id;
  uint32_t fftlen;
  uint32_t inchannels;
  uint32_t outchannels;
  bool autoconnect;
  std::string connect;
  std::string hrirfile;
  std::vector<channel_entry_t> matrix;
};

hrirconv_var_t::hrirconv_var_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    fftlen(1024),
    inchannels(0),outchannels(0),autoconnect(false)
{
  GET_ATTRIBUTE(id);
  if( id.empty() )
    id = "hrirconv";
  GET_ATTRIBUTE(fftlen);
  GET_ATTRIBUTE(inchannels);
  GET_ATTRIBUTE(outchannels);
  get_attribute_bool("autoconnect",autoconnect);
  GET_ATTRIBUTE(connect);
  GET_ATTRIBUTE(hrirfile);
  if( inchannels == 0 )
    throw TASCAR::ErrMsg("At least one input channel required");
  if( outchannels == 0 )
    throw TASCAR::ErrMsg("At least one output channel required");
  if( hrirfile.empty() ){
    xmlpp::Node::NodeList subnodes = e->get_children();
    for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
      xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
      if( sne && ( sne->get_name() == "entry" )){
        channel_entry_t che;
        get_attribute_value(sne,"in",che.inchannel);
        get_attribute_value(sne,"out",che.outchannel);
        che.filename = sne->get_attribute_value("file");
        get_attribute_value(sne,"channel",che.filechannel);
        matrix.push_back(che);
      }
    }
  }else{
    for(uint32_t kin=0;kin<inchannels;++kin){
      for(uint32_t kout=0;kout<outchannels;++kout){
        channel_entry_t che;
        che.inchannel = kin;
        che.outchannel = kout;
        che.filename = hrirfile;
        che.filechannel = kout + outchannels*kin;
        matrix.push_back(che);
      }
    }
  }
}

class hrirconv_mod_t : public hrirconv_var_t, public hrirconv_t {
public:
  hrirconv_mod_t( const TASCAR::module_cfg_t& cfg );
  void prepare( chunk_cfg_t& );
  virtual ~hrirconv_mod_t();
};


hrirconv_mod_t::hrirconv_mod_t( const TASCAR::module_cfg_t& cfg )
  : hrirconv_var_t( cfg ),
    hrirconv_t(inchannels,outchannels,fftlen, matrix, id, *this )
{
}

void hrirconv_mod_t::prepare( chunk_cfg_t& cf_ )
{
  module_base_t::prepare( cf_ );
  if( autoconnect ){
    for(std::vector<TASCAR::scene_render_rt_t*>::iterator iscenes=session->scenes.begin();
        iscenes != session->scenes.end();
        ++iscenes){
      for(std::vector<TASCAR::Scene::receivermod_object_t*>::iterator irec=(*iscenes)->receivermod_objects.begin();
          irec!=(*iscenes)->receivermod_objects.end(); ++irec){
        std::string prefix("render."+(*iscenes)->name+":");
        if((*irec)->get_num_channels() == inchannels){
          for(uint32_t ch=0;ch<inchannels;ch++){
            std::string pn(prefix+(*irec)->get_name()+(*irec)->get_channel_postfix(ch));
            connect_in(ch,pn,true);
          }
        }
      }
    }
  }else{
    // use connect variable:
    if( ! hrirconv_var_t::connect.empty() ){
      const char **pp_ports(jack_get_ports(jc, hrirconv_var_t::connect.c_str(), NULL, 0));
      if( pp_ports ){
        uint32_t ip(0);
        while( (*pp_ports) && (ip < get_num_input_ports())){
          connect_in(ip,*pp_ports,true,true);
          ++pp_ports;
          ++ip;
        }
      }else{
        TASCAR::add_warning("No port \""+hrirconv_var_t::connect+"\" found.");
      }
    }
  }
}

hrirconv_mod_t::~hrirconv_mod_t()
{
}

REGISTER_MODULE(hrirconv_mod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
