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

void TASCAR::xml_write_scene(const std::string& filename, scene_t scene)
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
  for( std::vector<src_object_t>::iterator src_it = scene.src.begin(); src_it != scene.src.end(); ++src_it){
    xmlpp::Element* src_node = root->add_child("src_object");
    src_node->set_attribute("name",src_it->name);
    sprintf(ctmp,"%g",src_it->start);
    src_node->set_attribute("start",ctmp);
    xmlpp::Element* sound_node = src_node->add_child("sound");
    sound_node->set_attribute("filename",src_it->sound.filename);
    sprintf(ctmp,"%g",src_it->sound.gain);
    sound_node->set_attribute("gain",ctmp);
    sprintf(ctmp,"%d",src_it->sound.channel);
    sound_node->set_attribute("channel",ctmp);
    sprintf(ctmp,"%d",src_it->sound.loop);
    sound_node->set_attribute("loop",ctmp);
    xmlpp::Element* pos_node = src_node->add_child("position");
    src_it->position.export_to_xml_element( pos_node );
  }
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
