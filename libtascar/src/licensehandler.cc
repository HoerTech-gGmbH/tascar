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

bool licensehandler_t::has_authors() const
{
  return !authors.empty();
}


bool licensehandler_t::distributable() const
{
  bool dist(true);
  for( auto it=licenses.begin();it!=licenses.end();++it)
    if( it->first == "unknown" )
      dist = false;
  return dist;
}

void licensehandler_t::add_author( const std::string& author, const std::string& tag )
{
  if( !author.empty() )
    authors[author].insert(tag);
}

void licensehandler_t::add_license( const std::string& license, const std::string& attribution, const std::string& tag )
{
  if( tag.empty() )
    TASCAR::add_warning("A license is specified without defining what is licensed.\n(\""+license+"\", \""+attribution+"\")");
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
  if( !authors.empty() ){
    retv += "Authors:\n";
    for( auto it=authors.begin();it!=authors.end();++it){
      retv += it->first;
      if( !it->second.empty() ){
        retv += " (";
        for( auto o=it->second.begin();o!=it->second.end();++o)
          retv += *o;
        retv += ")";
      }
      retv += "\n";
    }
    retv += "\n";
  }
  if( distributable() ){
    retv += "This file can be used and distributed according to following license conditions:\n\n";
  }else{
    if( use_markup )
      retv += "<h1>";
    retv += "Do not use or distribute this file!";
    if( use_markup )
      retv += "</h1>";
    retv += "\n";
    retv += "The license of some content is not defined.";
    retv += "\n";
  }
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
  retv += "\nWhen using TASCAR in scientific research, please always cite this publication:\n\nGrimm, Giso; Luberadzka, Joanna; Hohmann, Volker. A Toolbox for Rendering Virtual Acoustic Environments in the Context of Audiology. Acta Acustica united with Acustica, Volume 105, Number 3, May/June 2019, pp. 566-578(13), doi:10.3813/AAA.919337\n";
  return retv;
}

std::string licensehandler_t::get_authors() const
{
  std::string retv;
  if( !authors.empty() ){
    for( auto it=authors.begin();it!=authors.end();++it){
      retv += it->first;
      if( !it->second.empty() ){
        retv += " (";
        for( auto o=it->second.begin();o!=it->second.end();++o)
          retv += *o;
        retv += ")";
      }
      retv += "\n";
    }
    retv += "\n";
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
  if( !distributable() )
    retv = "Do not use or distribute this file!\n\n"+retv;
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
