#include "session_reader.h"
#include "errorhandling.h"
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

TASCAR::tsc_reader_t::tsc_reader_t()
    : xml_element_t(root), licensed_component_t(typeid(*this).name()),
      file_name("")
{
  // avoid problems with number format in xml file:
  setlocale(LC_ALL, "C");
  char* c_respath(getcwd(NULL, 0));
  session_path = c_respath;
  free(c_respath);
  if(get_element_name() != "session")
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got " +
                         get_element_name() + ".");
}

void add_includes(tsccfg::node_t e, const std::string& parentdoc,
                  licensehandler_t* lh)
{
  for(auto sne : tsccfg::node_get_children(e)) {
    if(tsccfg::node_get_name(sne) == "include") {
      std::string idocname(
          TASCAR::env_expand(tsccfg::node_get_attribute_value(sne, "name")));
      if((!idocname.empty()) && (idocname != parentdoc)) {
        TASCAR::xml_doc_t idoc(idocname, TASCAR::xml_doc_t::LOAD_FILE);
        if(tsccfg::node_get_name(idoc.root) !=
           tsccfg::node_get_name(e)) {
          throw TASCAR::ErrMsg("Invalid root node \"" +
                               tsccfg::node_get_name(idoc.root) +
                               "\" in include file \"" + idocname +
                               "\".\nexpected \"" + tsccfg::node_get_name(e) +
                               "\".");
        }
        std::string sublicense;
        std::string subattribution;
        get_license_info(idoc.root, "", sublicense, subattribution);
        lh->add_license(sublicense, subattribution,
                        TASCAR::tscbasename(idocname));
        add_includes(idoc.root, idocname, lh);
        for(auto isne : tsccfg::node_get_children(idoc.root)) {
          e->import_node(isne);
        }
      }
    } else {
      add_includes(sne, parentdoc, lh);
    }
  }
}

TASCAR::tsc_reader_t::tsc_reader_t(const std::string& filename_or_data,
                                   load_type_t t, const std::string& path)
    : xml_doc_t(filename_or_data, t), xml_element_t(doc->get_root_node()),
      licensed_component_t(typeid(*this).name()),
      file_name(((t == LOAD_FILE) ? filename_or_data : ""))
{
  switch(t) {
  case LOAD_FILE:
    file_name = TASCAR::env_expand(filename_or_data);
    break;
  case LOAD_STRING:
    file_name = "(loaded from string)";
    break;
  }
  // avoid problems with number format in xml file:
  setlocale(LC_ALL, "C");
  if(path.size()) {
    char c_fname[path.size() + 1];
    char c_respath[PATH_MAX];
    memcpy(c_fname, path.c_str(), path.size() + 1);
    session_path = realpath(dirname(c_fname), c_respath);
    if(chdir(session_path.c_str()) != 0)
      add_warning("Unable to change directory.");
  } else {
    char c_respath[PATH_MAX];
    session_path = getcwd(c_respath, PATH_MAX);
  }
  if(get_element_name() != "session")
    throw TASCAR::ErrMsg("Invalid root node name. Expected \"session\", got " +
                         get_element_name() + ".");
  // add session-includes:
  add_includes(e, "", this);
}

void TASCAR::tsc_reader_t::read_xml()
{
  GET_ATTRIBUTE(license, "", "license type");
  GET_ATTRIBUTE(attribution, "", "attribution of license, if applicable");
  add_license(license, attribution, "session file");
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn = subnodes.begin();
      sn != subnodes.end(); ++sn) {
    tsccfg::node_t sne(dynamic_cast<tsccfg::node_t>(*sn));
    if(sne) {
      if(tsccfg::node_get_name(sne) == "scene")
        add_scene(sne);
      else if(tsccfg::node_get_name(sne) == "range")
        add_range(sne);
      else if(tsccfg::node_get_name(sne) == "connect")
        add_connection(sne);
      else if(tsccfg::node_get_name(sne) == "modules") {
        xmlpp::Node::NodeList lsubnodes = sne->get_children();
        for(xmlpp::Node::NodeList::iterator lsn = lsubnodes.begin();
            lsn != lsubnodes.end(); ++lsn) {
          tsccfg::node_t lsne(dynamic_cast<tsccfg::node_t>(*lsn));
          if(lsne)
            add_module(lsne);
        }
      } else if(tsccfg::node_get_name(sne) == "license") {
        TASCAR::xml_element_t lic(sne);
        std::string license;
        std::string attribution;
        std::string name;
        lic.GET_ATTRIBUTE(license, "", "license type");
        lic.GET_ATTRIBUTE(attribution, "",
                          "attribution of license, if applicable");
        lic.GET_ATTRIBUTE(name, "", "name of licensed component");
        add_license(license, attribution, name);
      } else if(tsccfg::node_get_name(sne) == "author") {
        TASCAR::xml_element_t lic(sne);
        std::string of;
        std::string name;
        lic.GET_ATTRIBUTE(name, "", "author name");
        lic.GET_ATTRIBUTE(of, "", "name of authored component");
        add_author(name, of);
      } else if(tsccfg::node_get_name(sne) == "bibitem") {
        xmlpp::NodeSet stxt(sne->find("text()"));
        for(auto it = stxt.begin(); it != stxt.end(); ++it) {
          xmlpp::TextNode* txt(dynamic_cast<xmlpp::TextNode*>(*it));
          if(txt)
            add_bibitem(txt->get_content());
        }
      } else if((tsccfg::node_get_name(sne) != "include") &&
                (tsccfg::node_get_name(sne) != "mainwindow") &&
                (tsccfg::node_get_name(sne) != "description"))
        add_warning("Invalid element: " + tsccfg::node_get_name(sne), sne);
      if(tsccfg::node_get_name(sne) == "module")
        add_module(sne);
    }
  }
}

const std::string& TASCAR::tsc_reader_t::get_session_path() const
{
  return session_path;
}

const std::string& TASCAR::tsc_reader_t::get_file_name() const
{
  return file_name;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
