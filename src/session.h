#ifndef SESSION_H
#define SESSION_H

#include "xmlconfig.h"
#include "scene.h"
#include "audioplayer.h"

namespace TASCAR {

  class xml_doc_t {
  public:
    xml_doc_t();
    xml_doc_t(const std::string& filename);
    void save(const std::string& filename);
  protected:
    xmlpp::DomParser domp;
    xmlpp::Document* doc;
  };

  class connection_t : public TASCAR::xml_element_t {
  public:
    connection_t(xmlpp::Element*);
    void write_xml();
    std::string src;
    std::string dest;
  };

  class range_t : public scene_node_base_t {
  public:
    range_t(xmlpp::Element* e);
    void write_xml();
    void prepare(double fs, uint32_t fragsize){};
    std::string name;
    double start;
    double end;
  };

  class session_t : public TASCAR::xml_doc_t, public TASCAR::xml_element_t, public jackc_portless_t {
  public:
    session_t();
    session_t(const std::string& filename);
  private:
    session_t(const session_t& src);
  public:
    virtual ~session_t();
    TASCAR::Scene::scene_t* add_scene(xmlpp::Element* e=NULL);
    TASCAR::range_t* add_range(xmlpp::Element* e=NULL);
    TASCAR::connection_t* add_connection(xmlpp::Element* e=NULL);
    void write_xml();
    void start();
    void stop();
    void run(bool &b_quit);
    double get_duration() const { return duration; };
    uint32_t get_active_pointsources() const;
    uint32_t get_total_pointsources() const;
    uint32_t get_active_diffusesources() const;
    uint32_t get_total_diffusesources() const;
    //double get_time() const;
    // configuration variables:
    std::string name;
    double duration;
    bool loop;
    std::vector<TASCAR::scene_player_t*> player;
    std::vector<TASCAR::range_t*> ranges;
    std::vector<TASCAR::connection_t*> connections;
  protected:
    // derived variables:
    std::string session_path;
  private:
    void read_xml();
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
