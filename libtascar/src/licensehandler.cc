#include "licensehandler.h"
#include <fstream>
#include "xmlconfig.h"

void get_license_info( xmlpp::Element* e, const std::string& fname, std::string& license, std::string& attribution )
{
  if( e->get_attribute( "license" ) )
    license = e->get_attribute_value("license");
  if( e->get_attribute( "attribution" ) )
    attribution = e->get_attribute_value("attribution");
  if( !fname.empty() ){
    std::ifstream flic(TASCAR::env_expand(fname)+".license");
    if( flic.good() ){
      if( !flic.eof())
        std::getline(flic,license);
      if( !flic.eof())
        std::getline(flic,attribution);
    }
  }  
}

void licensehandler_t::add_license( const std::string& license, const std::string& attribution, const std::string& tag )
{
  if( (license.find("CC")!=std::string::npos ) && 
      (license.find("BY")!=std::string::npos))
    if( attribution.empty() )
      TASCAR::add_warning("Empty attribution in license \""+license+"\".");
  if( license.empty() )
    licenses["unknown"].insert(tag);
  else
    licenses[license].insert(tag);
  if( !attribution.empty() )
    attributions[attribution].insert(tag);
}

std::string licensehandler_t::legal_stuff( bool use_markup ) const
{
  std::string retv;
  if( use_markup )
    retv += "<b>";
  retv += "Licenses:";
  if( use_markup )
    retv += "</b>";
  retv += "\n";
  for(std::map<std::string,std::set<std::string> >::const_iterator it=licenses.begin();it!=licenses.end();++it){
    retv += it->first + " ";
    for(std::set<std::string>::const_iterator sit=it->second.begin();sit!=it->second.end();++sit){
      if( sit==it->second.begin() )
        retv += "(";
      else
        retv += ", ";
      retv += *sit;
    }
    if( !it->second.empty() )
      retv += ")";
    retv += "\n";
  }
  if( !attributions.empty() ){
    retv += "\n";
    if( use_markup )
      retv += "<b>";
    retv += "Attributions:";
    if( use_markup )
      retv += "</b>";
    retv += "\n";
    for(std::map<std::string,std::set<std::string> >::const_iterator it=attributions.begin();it!=attributions.end();++it){
      retv += it->first + " ";
      for(std::set<std::string>::const_iterator sit=it->second.begin();sit!=it->second.end();++sit){
        if( sit==it->second.begin() )
          retv += "(";
        else
          retv += ", ";
        retv += *sit;
      }
      if( !it->second.empty() )
        retv += ")";
      retv += "\n";
    }
  }
  return retv;
}


std::string licensehandler_t::show_unknown() const
{
  std::string retv;
  for(std::map<std::string,std::set<std::string> >::const_iterator it=licenses.begin();it!=licenses.end();++it){
    if( it->first == "unknown" ){
      for(std::set<std::string>::const_iterator sit=it->second.begin();sit!=it->second.end();++sit){
        if( sit!=it->second.begin() )
          retv += ", ";
        retv += *sit;
      }
    }
  }
  if( !retv.empty() )
    retv = "Unknown licenses: "+retv;
  return retv;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
