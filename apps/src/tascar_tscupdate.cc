/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libxml++/libxml++.h>
#include <iostream>
#include <stdio.h>
#include <math.h>

xmlpp::Element* find_or_add_child( xmlpp::Element* e, const std::string& name )
{
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == name ))
      return sne;
  }
  return e->add_child(name);
}

void copy_attrib(xmlpp::Element* src,xmlpp::Element* dest,const std::string& name)
{
  if( !src->get_attribute_value(name).empty() )
    dest->set_attribute(name,src->get_attribute_value(name));
}

void scan_sound( xmlpp::Element* sound )
{
  xmlpp::Node::NodeList children(sound->get_children());
  for(xmlpp::Node::NodeList::iterator nita=children.begin();nita!=children.end();++nita){
    xmlpp::Element* e_child(dynamic_cast<xmlpp::Element*>(*nita));
    if( e_child ){
      if( e_child && (e_child->get_name() != "plugins") ){
        if( e_child->get_name() == "plugin" ){
          std::string plugtype(e_child->get_attribute_value("type"));
          e_child->set_name(plugtype);
          e_child->remove_attribute("type");
        }
        xmlpp::Element* plugs(find_or_add_child( sound, "plugins" ));
        plugs->import_node(e_child);
        sound->remove_child(e_child);
        e_child = NULL;
      }
    }
  }
}

void scan_source(xmlpp::Element* src)
{
  xmlpp::Node::NodeList soundchildren(src->get_children("sound"));
  uint32_t num_sounds(soundchildren.size());
  xmlpp::Node::NodeList children(src->get_children());
  for(xmlpp::Node::NodeList::iterator nita = children.begin();
      nita != children.end(); ++nita) {
    xmlpp::Element* e_child(dynamic_cast<xmlpp::Element*>(*nita));
    if(e_child) {
      if(e_child && (e_child->get_name() == "sound"))
        scan_sound(e_child);
      if(e_child && (e_child->get_name() == "sndfile")) {
        xmlpp::Element* snd;
        if(num_sounds == 1)
          snd = find_or_add_child(src, "sound");
        else
          snd = find_or_add_child(src, "move_this_to_the_right_sound");
        xmlpp::Element* plugs(find_or_add_child(snd, "plugins"));
        xmlpp::Element* ne_child(plugs->add_child("sndfile"));
        std::string fchannel(e_child->get_attribute_value("firstchannel"));
        if(!fchannel.empty())
          ne_child->set_attribute("channel", fchannel);
        ne_child->set_attribute("name", e_child->get_attribute_value("name"));
        std::string sloop(e_child->get_attribute_value("loop"));
        if(!sloop.empty())
          ne_child->set_attribute("loop", sloop);
        // e_child->remove_attribute("firstchannel");
        ne_child->set_attribute("levelmode", "calib");
        // gain to level conversion:
        std::string gain(e_child->get_attribute_value("gain"));
        double dgain(0);
        if(!gain.empty())
          dgain = atof(gain.c_str());
        char ctmp[1024];
        ctmp[1023] = 0;
        snprintf(ctmp, 1023, "%g", dgain - 20.0 * log10(2e-5));
        ne_child->set_attribute("level", ctmp);
        // e_child->remove_attribute("gain");
        // starttime to position conversion:
        std::string startt(e_child->get_attribute_value("starttime"));
        double dstart(0);
        if(!startt.empty()) {
          dstart = atof(startt.c_str());
          snprintf(ctmp, 1023, "%g", -dstart);
          ne_child->set_attribute("position", ctmp);
        }
        ne_child->remove_attribute("starttime");
        std::string sconnect(snd->get_attribute_value("connect"));
        if((sconnect == "@.0") || (sconnect == "@.1")) {
          snd->remove_attribute("connect");
        }
        // copy:
        // plugs->import_node(e_child);
        src->remove_child(e_child);
        e_child = NULL;
      }
    }
  }
}

void scan_scene( xmlpp::Element* scene, xmlpp::Element* session )
{
  scene->remove_attribute("duration");
  scene->remove_attribute("loop");
  scene->remove_attribute("lat");
  scene->remove_attribute("lon");
  scene->remove_attribute("elev");
  xmlpp::Node::NodeList children(scene->get_children());
  for(xmlpp::Node::NodeList::iterator nita=children.begin();nita!=children.end();++nita){
    xmlpp::Element* e_child(dynamic_cast<xmlpp::Element*>(*nita));
    if( e_child ){
      if( e_child && (e_child->get_name() == "sink") )
        e_child->set_name("receiver");
      if( e_child && (e_child->get_name() == "src_object") )
        e_child->set_name("source");
      if( e_child && (e_child->get_name() == "connect") ){
        session->import_node(e_child);
        scene->remove_child(e_child);
        e_child = NULL;
      }
      if( e_child && (e_child->get_name() == "range") ){
        session->import_node(e_child);
        scene->remove_child(e_child);
        e_child = NULL;
      }
      if( e_child && (e_child->get_name() == "description") ){
        session->import_node(e_child);
        scene->remove_child(e_child);
        e_child = NULL;
      }
      if( e_child && (e_child->get_name() == "source") )
        scan_source( e_child );
    }
  }
}

void del_whitespace( xmlpp::Node* node )
{
    xmlpp::TextNode* nodeText = dynamic_cast<xmlpp::TextNode*>(node);
    if( nodeText && nodeText->is_white_space()){
	nodeText->get_parent()->remove_child(node);
    }else{
	xmlpp::Element* nodeElement = dynamic_cast<xmlpp::Element*>(node);
	if( nodeElement ){
	    xmlpp::Node::NodeList children = nodeElement->get_children();
	    for(xmlpp::Node::NodeList::iterator nita=children.begin();nita!=children.end();++nita){
		del_whitespace( *nita );
	    }
	}
    }
}

void scan_session( xmlpp::Element* session )
{
  session->set_attribute("license",session->get_attribute_value("license"));
  session->set_attribute("attribution",session->get_attribute_value("attribution"));
  xmlpp::Node::NodeList children(session->get_children());
  for(xmlpp::Node::NodeList::iterator nita=children.begin();nita!=children.end();++nita){
    xmlpp::Element* e_child(dynamic_cast<xmlpp::Element*>(*nita));
    if( e_child ){
      if( e_child && (e_child->get_name() == "scene") )
        scan_scene( e_child, session );
      if( e_child && (e_child->get_name() == "module") ){
        xmlpp::Element* modules(find_or_add_child( session, "modules" ));
        std::string modname(e_child->get_attribute_value("name"));
        e_child->set_name(modname);
        e_child->remove_attribute( "name" );
        modules->import_node(e_child);
        session->remove_child(e_child);
        e_child = NULL;
      }
    }
  }
}

int main(int argc,char** argv)
{
  if( argc != 2 ){
    std::cerr << "Usage: tascar_tscupdate <infile>\n";
    return 1;
  }
  xmlpp::DomParser domp(argv[1]);
  xmlpp::Document* doc(domp.get_document());
  xmlpp::Element* session(doc->get_root_node());
  if( session->get_name() != "session" ){
      std::cerr << "Error: No tascar file format detected (top element is not \"session\").\n";
      return 1;
  }
  scan_session( session );
  del_whitespace( session );
  std::string oname(std::string(argv[1])+".updated");
  doc->write_to_file_formatted( oname.c_str() );
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
