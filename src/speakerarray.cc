#include "speakerarray.h"
#include "errorhandling.h"
#include <complex.h>
#include <algorithm>

using namespace TASCAR;

// speaker array:

spk_array_t::spk_array_t(xmlpp::Element* e, const std::string& elementname_)
  : xml_element_t(e),
    rmax(0),
    rmin(0),
    xyzgain(1.0),
    elementname(elementname_),
    mean_rotation(0)
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
    double _Complex p_xy(0);
    for(uint32_t k=0;k<size();k++){
      operator[](k).gain = operator[](k).norm()/rmax;
      operator[](k).dr = rmax-operator[](k).norm();
      p_xy += cexp(-(double)k*PI2*I/(double)(size()))*(operator[](k).unitvector.x+I*operator[](k).unitvector.y);
    }
    mean_rotation = carg(p_xy);
  }
  if( empty() )
    throw TASCAR::ErrMsg("Invalid empty speaker array.");
  didx.resize(size());
  for(uint32_t k=0;k<size();k++){
    operator[](k).update_foa_decoder(1.0f/size(), xyzgain);
    connections.push_back(operator[](k).connect);
  }
}

void spk_array_t::read_xml(xmlpp::Element* e)
{
  GET_ATTRIBUTE(xyzgain);
  clear();
  std::string importsrc;
  xml_element_t xe(e);
  xe.get_attribute("layout",importsrc);
  if( !importsrc.empty() )
    import_file(importsrc);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == elementname )){
      push_back(TASCAR::spk_pos_t(sne));
    }
  }
}

void spk_array_t::import_file(const std::string& fname)
{
  try{
    domp.parse_file(TASCAR::env_expand(fname));
  }
  catch( const xmlpp::internal_error& e){
    throw TASCAR::ErrMsg(std::string("xml internal error: ")+e.what());
  }
  catch( const xmlpp::validity_error& e){
    throw TASCAR::ErrMsg(std::string("xml validity error: ")+e.what());
  }
  catch( const xmlpp::parse_error& e){
    throw TASCAR::ErrMsg(std::string("xml parse error: ")+e.what());
  }
  if( !domp )
    throw TASCAR::ErrMsg("Unable to parse file \""+fname+"\".");
  xmlpp::Element* r(domp.get_document()->get_root_node());
  if( !r )
    throw TASCAR::ErrMsg("No root node found in document \""+fname+"\".");
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
  update_foa_decoder(1.0f, 1.0 );
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

void spk_pos_t::update_foa_decoder(float gain, double xyzgain)
{
  // update of FOA decoder matrix:
  d_w = 1.4142135623730951455f * gain;
  gain *= xyzgain;
  d_x = unitvector.x * gain;
  d_y = unitvector.y * gain;
  d_z = unitvector.z * gain;
}

void spk_array_t::prepare( chunk_cfg_t& cf_ )
{
  audiostates_t::prepare( cf_ );
  delaycomp.clear();
  for(uint32_t k=0;k<size();++k)
    delaycomp.push_back(TASCAR::static_delay_t( f_sample*(operator[](k).dr/340.0)));
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
