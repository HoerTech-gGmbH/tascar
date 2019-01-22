#include "session.h"
#include <lsl_cpp.h>

class pos2lsl_t : public TASCAR::module_base_t {
public:
  pos2lsl_t( const TASCAR::module_cfg_t& cfg );
  ~pos2lsl_t();
  virtual void prepare( chunk_cfg_t& );
  void update(uint32_t frame, bool running);
private:
  std::string pattern;
  bool transport;
  std::vector<TASCAR::named_object_t> obj;
  std::vector<lsl::stream_outlet*> voutlet;
  std::vector<float> data;
};

pos2lsl_t::pos2lsl_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    transport(true)
{
  data.resize(6);
  GET_ATTRIBUTE(pattern);
  GET_ATTRIBUTE_BOOL(transport);
  if( pattern.empty() )
    pattern = "/*/*";
  obj = session->find_objects(pattern);
  if( !obj.size() )
    throw TASCAR::ErrMsg("No target objects found (target pattern: \""+pattern+"\").");
}

void pos2lsl_t::prepare( chunk_cfg_t& cf_ )
{
  module_base_t::prepare( cf_ );
  for(auto it=voutlet.begin();it!=voutlet.end();++it)
    delete *it;
  voutlet.clear();
  for(auto it=obj.begin();it!=obj.end();++it)
    voutlet.push_back(new lsl::stream_outlet(lsl::stream_info(it->obj->get_name(),"position",6,cf_.f_fragment)));
}

pos2lsl_t::~pos2lsl_t()
{
  for(uint32_t k=0;k<voutlet.size();++k)
    delete voutlet[k];
}

void pos2lsl_t::update(uint32_t tp_frame, bool tp_rolling)
{
  if( tp_rolling || (!transport) ){
    uint32_t kobj(0);
    for(auto it=obj.begin();it!=obj.end();++it){
      TASCAR::pos_t p;
      TASCAR::zyx_euler_t o;
      it->obj->get_6dof(p,o);
      data[0] = p.x;
      data[1] = p.y;
      data[2] = p.z;
      data[3] = RAD2DEG*o.z;
      data[4] = RAD2DEG*o.y;
      data[5] = RAD2DEG*o.x;
      voutlet[kobj]->push_sample(data);
      ++kobj;
    }
  }
}

REGISTER_MODULE(pos2lsl_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
