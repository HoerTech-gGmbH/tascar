function h = tascar_xml_get_el_handle( doc, etype, varargin )
% Get a handle to the element that you want
%
% Usage:
% h = tascar_xml_edit_elements( doc, etype,[cattr1, cvalue1, ...] )
% 
% doc - document (java object)
% etype - element name
% cattr1 - constraint attribute name
% cvalue1 - constraint attribute value
%
% Example:
% Open file 'input.xml', add sub-element 'src_object' in <group.../> elements with
% attribute 'name' = 'alpha' and attribute 'id' = '5':
%
% doc = tascar_xml_open('input.xml');
% el_handle = tascar_xml_edit_elements( doc, 'group', 'name', 'alpha', 'id', '5')
% src_handle = tascar_xml_add_element( doc, el_handle, 'src_object', [], 'name','new_source') 
% tascar_xml_save(doc, 'output.xml');
%
  root = javaMethod('getFirstChild',doc);
  elem_list = javaMethod('getElementsByTagName',doc,etype);
  N = javaMethod('getLength',elem_list);
  for k=1:N
    elem = javaMethod('item',elem_list,k-1);
    b_matched = true;
    for k=1:2:numel(varargin)
      if ~strcmp(javaMethod('getAttribute',elem,varargin{k}),varargin{k+1})
	b_matched = false;
      end
    end
    if b_matched
     h=elem;
    end
  end
