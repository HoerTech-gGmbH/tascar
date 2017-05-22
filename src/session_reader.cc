#include "session_reader.h"
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include "errorhandling.h"

TASCAR::tsc_reader_t::tsc_reader_t()
  : xml_element_t(doc->get_root_node())
{
  // avoid problems with number format in xml file:
  setlocale(LC_ALL,"C");
  char c_respath[PATH_MAX];
  session_path = getcwd(c_respath,PATH_MAX);
  if( get_element_name() != "session" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got "+get_element_name()+".");
}

void add_includes( xmlpp::Element* e, const std::string& parentdoc )
{
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      if( sne->get_name() == "include" ){
        std::string idocname(sne->get_attribute_value("name"));
        if( (!idocname.empty()) && (idocname != parentdoc) ){
          TASCAR::xml_doc_t idoc(idocname,TASCAR::xml_doc_t::LOAD_FILE);
          if( idoc.doc->get_root_node()->get_name() != e->get_name() ){
            throw TASCAR::ErrMsg("Invalid root node \""+idoc.doc->get_root_node()->get_name()+"\" in include file \""+idocname+"\".\nexpected \""+e->get_name()+"\".");
          }
          add_includes( idoc.doc->get_root_node(), idocname );
          xmlpp::Node::NodeList isubnodes = idoc.doc->get_root_node()->get_children();
          for(xmlpp::Node::NodeList::iterator isn=isubnodes.begin();isn!=isubnodes.end();++isn){
            xmlpp::Element* isne(dynamic_cast<xmlpp::Element*>(*isn));
            if( isne ){
              e->import_node( isne );
            }
          }
        }
      }else{
        add_includes( sne, parentdoc );
      }
    }
  }
}

TASCAR::tsc_reader_t::tsc_reader_t(const std::string& filename_or_data,load_type_t t,const std::string& path)
  : xml_doc_t(filename_or_data,t),
    xml_element_t(doc->get_root_node())
{
  // avoid problems with number format in xml file:
  setlocale(LC_ALL,"C");
  if( path.size() ){
    char c_fname[path.size()+1];
    char c_respath[PATH_MAX];
    memcpy(c_fname,path.c_str(),path.size()+1);
    session_path = realpath(dirname(c_fname),c_respath);
    if( chdir(session_path.c_str()) != 0 )
      std::cerr << "Unable to change directory\n";
  }else{
    char c_respath[PATH_MAX];
    session_path = getcwd(c_respath,PATH_MAX);
  }
  if( get_element_name() != "session" )
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got "+get_element_name()+".");
  // add session-includes:
  add_includes( e, "" );
  //xmlpp::Node::NodeList subnodes = e->get_children();
  //for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
  //  xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
  //  if( sne && ( sne->get_name() == "include")){
  //    std::string idocname(sne->get_attribute_value("name"));
  //    if( !idocname.empty() ){
  //      xml_doc_t idoc(idocname,LOAD_FILE);
  //      xmlpp::Node::NodeList isubnodes = idoc.doc->get_root_node()->get_children();
  //      for(xmlpp::Node::NodeList::iterator isn=isubnodes.begin();isn!=isubnodes.end();++isn){
  //        xmlpp::Element* isne(dynamic_cast<xmlpp::Element*>(*isn));
  //        if( isne ){
  //          e->import_node(isne);
  //        }
  //      }
  //    }
  //  }
  //}
}

void TASCAR::tsc_reader_t::read_xml()
{
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "scene"))
      add_scene(sne);
    if( sne && ( sne->get_name() == "range"))
      add_range(sne);
    if( sne && ( sne->get_name() == "connect"))
      add_connection(sne);
    if( sne && ( sne->get_name() == "module"))
      add_module(sne);
  }
}

void TASCAR::tsc_reader_t::write_xml()
{
}


void TASCAR::tsc_reader_t::save(const std::string& filename)
{
  write_xml();
  xml_doc_t::save(filename);
}

const std::string& TASCAR::tsc_reader_t::get_session_path() const
{
  return session_path;
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
