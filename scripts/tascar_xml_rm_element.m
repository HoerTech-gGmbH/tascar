function doc = tascar_xml_rm_element(  doc, pNode, etype, varargin )
% tascar_xml_edit_elements - edit XML elements
%
% Usage:
% doc = tascar_xml_edit_elements( doc, etype, [cattr1, cvalue1, ...] )
% 
% doc - document (java object)
% pNode - parent node of the element to be removed
% cName - name of the new element
% cattr1 - constraint attribute name
% cvalue1 - constraint attribute value
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
      doc=javaMethod('removeChild',pNode, elem);
    end
  end
