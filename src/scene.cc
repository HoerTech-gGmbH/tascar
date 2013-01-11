#include "scene.h"
#include <libxml++/libxml++.h>
#include <stdio.h>

using namespace TASCAR;

scene_t::scene_t()
  : name(""),
    lat(53.155473),
    lon(8.167249),
    elev(10)
{
}

listener_t::listener_t()
{
}

src_object_t::src_object_t()
  : name(""),
    start(0)
{
}

bg_amb_t::bg_amb_t()
  : start(0),
    filename(""),
    gain(0),
    loop(1)
{
}

sound_t::sound_t()
  : filename(""),
    gain(0),
    channel(0),
    loop(1)
{
}

scene_t TASCAR::xml_read_scene(const std::string& filename)
{
    scene_t s;
    return s;
}

void TASCAR::xml_write_scene(const std::string& filename, const scene_t& scene)
{ 
  char ctmp[1024];
  xmlpp::Document doc;
  xmlpp::Element* root(doc.create_root_node("scene"));
  root->set_attribute("name",scene.name);
  sprintf(ctmp,"%g",scene.lat);
  root->set_attribute("lat",ctmp);
  sprintf(ctmp,"%g",scene.lon);
  root->set_attribute("lon",ctmp);
  sprintf(ctmp,"%g",scene.elev);
  root->set_attribute("elev",ctmp);
  doc.write_to_file_formatted(filename);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
