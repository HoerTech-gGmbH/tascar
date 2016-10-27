#include "speakerarray.h"
#include "errorhandling.h"
#include <complex.h>
#include <algorithm>

using namespace TASCAR;

// speaker array:

spk_array_t::spk_array_t(xmlpp::Element* e)
  : xml_element_t(e),
    rmax(0),
    rmin(0)
{
  read_xml(e);
  if( !empty() ){
    rmax = operator[](0).norm();
    rmin = rmax;
    for(uint32_t k=1;k<size();k++){
      double r(operator[](k).norm());
      if( rmax < r )
        rmax = r;
      if( rmin > r )
        rmin = r;
    }
    for(uint32_t k=0;k<size();k++){
      operator[](k).gain = operator[](k).norm()/rmax;
      operator[](k).dr = rmax-operator[](k).norm();
    }
  }
  if( empty() )
    throw TASCAR::ErrMsg("Invalid empty speaker array.");
  didx.resize(size());
  for(uint32_t k=0;k<size();k++){
    operator[](k).update_foa_decoder(1.0f/size());
    connections.push_back(operator[](k).connect);
  }
}

void spk_array_t::read_xml(xmlpp::Element* e)
{
  clear();
  std::string importsrc(e->get_attribute_value("layout"));
  if( !importsrc.empty() )
    import_file(importsrc);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "speaker" )){
      push_back(TASCAR::spk_pos_t(sne));
    }
  }
}

void spk_array_t::write_xml()
{
}

void spk_array_t::import_file(const std::string& fname)
{
  xmlpp::DomParser domp;
  domp.parse_file(TASCAR::env_expand(fname));
  xmlpp::Element* r(domp.get_document()->get_root_node());
  if( r->get_name() != "layout" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"layout\", got "+r->get_name()+".");
  read_xml(r);
}

double spk_pos_t::get_rel_azim(double az_src) const
{
  return carg(cexp( I *(az_src - az)));
}

double spk_pos_t::get_cos_adist(pos_t src_unit) const
{
  return dot_prod( src_unit, unitvector );
}

bool sort_didx(const spk_array_t::didx_t& a,const spk_array_t::didx_t& b)
{
  return (a.d > b.d);
}

const std::vector<spk_array_t::didx_t>& spk_array_t::sort_distance(const pos_t& psrc)
{
  for(uint32_t k=0;k<size();++k){
    didx[k].idx = k;
    didx[k].d = dot_prod(psrc,operator[](k).unitvector);
  }
  std::sort(didx.begin(),didx.end(),sort_didx);
  return didx;
}

void spk_array_t::foa_decode(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output)
{
  uint32_t channels(size());
  if( output.size() != channels ){
    throw TASCAR::ErrMsg("Invalid size of speaker array");
  }
  uint32_t N(chunk.size());
  for( uint32_t t=0; t<N; ++t ){
    for( uint32_t ch=0; ch<channels; ++ch ){
      output[ch][t] += operator[](ch).d_w * chunk.w()[t];
      output[ch][t] += operator[](ch).d_x * chunk.x()[t];
      output[ch][t] += operator[](ch).d_y * chunk.y()[t];
      output[ch][t] += operator[](ch).d_z * chunk.z()[t];
    }
  }
}

spk_pos_t::spk_pos_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),
    az(0.0),
    el(0.0),
    r(1.0),
    gain(1.0),
    dr(0.0),      
    d_w(0.0f),
    d_x(0.0f),
    d_y(0.0f),
    d_z(0.0f),
    comp(NULL)
{
  GET_ATTRIBUTE_DEG(az);
  GET_ATTRIBUTE_DEG(el);
  GET_ATTRIBUTE(r);
  GET_ATTRIBUTE(label);
  GET_ATTRIBUTE(connect);
  GET_ATTRIBUTE(compA);
  GET_ATTRIBUTE(compB);
  set_sphere(r,az,el);
  unitvector = normal();
  update_foa_decoder(1.0f);
  if( (compA.size() > 0) && (compB.size() > 0) )
    comp = new TASCAR::filter_t( compA, compB );
}

spk_pos_t::spk_pos_t(const spk_pos_t& src)
  : xml_element_t(src),
    pos_t(src),
    az(src.az),
    el(src.el),
    r(src.r),
    label(src.label),
    connect(src.connect),
    compA(src.compA),
    compB(src.compB),
    unitvector(src.unitvector),
    gain(src.gain),
    dr(src.dr),      
    d_w(src.d_w),
    d_x(src.d_x),
    d_y(src.d_y),
    d_z(src.d_z),
    comp(NULL)
{
  if( (compA.size() > 0) && (compB.size() > 0) )
    comp = new TASCAR::filter_t( compA, compB );
}

spk_pos_t::~spk_pos_t()
{
  if( comp )
    delete comp;
}

void spk_pos_t::update_foa_decoder(float gain)
{
  // update of FOA decoder matrix:
  TASCAR::pos_t px(1,0,0);
  TASCAR::pos_t py(0,1,0);
  TASCAR::pos_t pz(0,0,1);
  d_w = sqrtf(0.5f) * gain;
  d_x = dot_prod(px,unitvector) * gain;
  d_y = dot_prod(py,unitvector) * gain;
  d_z = dot_prod(pz,unitvector) * gain;
}

void spk_pos_t::write_xml()
{
  if( az != 0.0 )
    SET_ATTRIBUTE_DEG(az);
  if( el != 0.0 )
    SET_ATTRIBUTE_DEG(el);
  if( r != 1.0 )
    SET_ATTRIBUTE(r);
  if( !label.empty() )
    SET_ATTRIBUTE(label);
  if( !connect.empty() )
    SET_ATTRIBUTE(connect);
}

void spk_array_t::configure(double srate, uint32_t fragsize)
{
  delaycomp.clear();
  for(uint32_t k=0;k<size();++k)
    delaycomp.push_back(TASCAR::static_delay_t(srate*(operator[](k).dr/340.0)));
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
