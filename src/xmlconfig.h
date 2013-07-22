#ifndef XMLCONFIG_H
#define XMLCONFIG_H

namespace TASCAR {

  class scene_node_base_t {
  public:
    scene_node_base_t(){};
    virtual ~scene_node_base_t(){};
    virtual void read_xml(xmlpp::Element* e) = 0;
    virtual void write_xml(xmlpp::Element* e,bool help_comments=false) = 0;
    //virtual std::string print(const std::string& prefix="") = 0;
    virtual void prepare(double fs, uint32_t fragsize) = 0;
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
