#ifndef SESSION_H
#define SESSION_H

#include "xmlconfig.h"
#include "scene.h"

namespace TASCAR {

  class xml_doc_t {
  public:
    xml_doc_t();
    xml_doc_t(const std::string& filename);
  protected:
    xmlpp::Document* doc;
  };

  class session_t : public TASCAR::xml_doc_t, public TASCAR::xml_element_t {
  public:
    session_t();
    session_t(const std::string& filename);
    ~session_t();
    TASCAR::Scene::scene_t& add_scene(xmlpp::Element* e=NULL);
    TASCAR::Scene::range_t& add_range(xmlpp::Element* e=NULL);
    TASCAR::Scene::connection_t& add_connection(xmlpp::Element* e=NULL);
  protected:
    // configuration variables:
    std::string name;
    double duration;
    bool loop;
    std::vector<TASCAR::Scene::scene_t> scenes;
    std::vector<TASCAR::Scene::range_t> ranges;
    std::vector<TASCAR::Scene::connection_t> connections;
    // derived variables:
    std::string session_path;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
