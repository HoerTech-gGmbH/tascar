#include "session.h"
#include <lsl_cpp.h>

class levels2osc_t : public TASCAR::module_base_t {
public:
  levels2osc_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
  ~levels2osc_t();
  void write_xml();
  virtual void configure(double srate,uint32_t fragsize);
  void update(uint32_t frame, bool running);
private:
  std::string url;
  std::string pattern;
  uint32_t ttl;
  lo_address target;
  std::vector<TASCAR::named_object_t> obj;
  std::vector<lo_message> vmsg;
  std::vector<lo_arg**> vargv;
  std::vector<std::string> vpath;
  std::vector<lsl::stream_outlet*> voutlet;
};

levels2osc_t::levels2osc_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
  : module_base_t(xmlsrc,session),
    ttl(1)
{
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(pattern);
  GET_ATTRIBUTE(ttl);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  if( pattern.empty() )
    pattern = "/*/*";
  target = lo_address_new_from_url(url.c_str());
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+url+"\".");
  lo_address_set_ttl(target,ttl);
  obj = session->find_objects(pattern);
  if( !obj.size() )
    throw TASCAR::ErrMsg("No target objects found (target pattern: \""+pattern+"\").");
}

void levels2osc_t::configure(double srate,uint32_t fragsize)
{
  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
    vmsg.push_back(lo_message_new());
    for(uint32_t k=0;k<it->obj->metercnt();++k)
      lo_message_add_float(vmsg.back(),0);
    vargv.push_back(lo_message_get_argv(vmsg.back()));
    vpath.push_back(std::string("/level/")+it->obj->get_name());
    voutlet.push_back(new lsl::stream_outlet(lsl::stream_info(it->obj->get_name(),"level",it->obj->metercnt())));
  }
}

void levels2osc_t::write_xml()
{
  SET_ATTRIBUTE(url);
  SET_ATTRIBUTE(pattern);
  SET_ATTRIBUTE(ttl);
}

levels2osc_t::~levels2osc_t()
{
  for(uint32_t k=0;k<vmsg.size();++k)
    lo_message_free(vmsg[k]);
  for(uint32_t k=0;k<voutlet.size();++k)
    delete voutlet[k];
  lo_address_free(target);
}

void levels2osc_t::update(uint32_t tp_frame, bool tp_rolling)
{
  uint32_t k=0;
  for(std::vector<TASCAR::named_object_t>::iterator it=obj.begin();it!=obj.end();++it){
    const std::vector<float>& leveldata(it->obj->readmeter());
    voutlet[k]->push_sample(leveldata);
    uint32_t n(it->obj->metercnt());
    for(uint32_t km=0;km<n;++km)
      vargv[k][km]->f = it->obj->get_meterval(km);
    lo_send_message( target, vpath[k].c_str(), vmsg[k] );
    ++k;
  }
}

REGISTER_MODULE(levels2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
