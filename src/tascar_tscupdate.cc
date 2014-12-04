#include <libxml++/libxml++.h>
#include <iostream>
#include <stdio.h>

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

void copy_attrib(xmlpp::Element* src,xmlpp::Element* dest,const std::string& name)
{
  if( !src->get_attribute_value(name).empty() )
    dest->set_attribute(name,src->get_attribute_value(name));
}

int main(int argc,char** argv)
{
  if( argc != 2 ){
    std::cerr << "Usage: tascar_tscupdate <infile>\n";
    return 1;
  }
  xmlpp::DomParser domp(argv[1]);
  xmlpp::Document* doc(domp.get_document());
  xmlpp::Element* root(doc->get_root_node());
  if( root->get_name() == "scene" ){
    std::cout << "Old format detected.\n";
    xmlpp::Document ndoc;
    xmlpp::Element* nroot(ndoc.create_root_node("session"));
    nroot->set_attribute("name","tascar");
    copy_attrib(root,nroot,"duration");
    copy_attrib(root,nroot,"loop");
    xmlpp::Node* scene(nroot->import_node(root));
    xmlpp::Element* escene(dynamic_cast<xmlpp::Element*>(scene));
    escene->remove_attribute("duration");
    escene->remove_attribute("loop");
    escene->remove_attribute("lat");
    escene->remove_attribute("lon");
    escene->remove_attribute("elev");
    xmlpp::Node::NodeList children(scene->get_children());
    for(xmlpp::Node::NodeList::iterator nita=children.begin();nita!=children.end();++nita){
      xmlpp::Element* nodeElement(dynamic_cast<xmlpp::Element*>(*nita));
      if( nodeElement && (nodeElement->get_name() == "sink") ){
        nodeElement->set_name("receiver");
      }
      if( nodeElement && (nodeElement->get_name() == "connect") ){
        nroot->import_node(nodeElement);
        scene->remove_child(nodeElement);
      }
      if( nodeElement && (nodeElement->get_name() == "range") ){
        nroot->import_node(nodeElement);
        scene->remove_child(nodeElement);
      }
      if( nodeElement && (nodeElement->get_name() == "description") ){
        nroot->import_node(nodeElement);
        scene->remove_child(nodeElement);
      }
    }
    //del_whitespace( root );
    std::string oname(std::string(argv[1])+".orig");
    rename(argv[1],oname.c_str() );
    del_whitespace( nroot );
    ndoc.write_to_file_formatted( argv[1] );
  }else{
    if( root->get_name() == "session" ){
      std::cout << "New format detected, nothing done.\n";
    }else{
      std::cerr << "Error: No tascar file format detected.\n";
      return 1;
    }
  }
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
