#ifndef SESSION_READER_H
#define SESSION_READER_H

#include "xmlconfig.h"

namespace TASCAR {

  class tsc_reader_t : public TASCAR::xml_doc_t, public xml_element_t {
  public:
    tsc_reader_t();
    tsc_reader_t(const std::string& filename_or_data,load_type_t t,const std::string& path);
    void read_xml();
    void write_xml();
    virtual void save(const std::string& filename);
    const std::string& get_session_path() const;
  private:
    tsc_reader_t(const tsc_reader_t&);
  protected:
    virtual void add_scene(xmlpp::Element* e) {};
    virtual void add_range(xmlpp::Element* e) {};
    virtual void add_connection(xmlpp::Element* e) {};
    virtual void add_module(xmlpp::Element* e) {};
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

