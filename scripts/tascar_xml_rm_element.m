function doc = tascar_xml_rm_element(  pNode, etype, varargin )
% tascar_xml_rm_element - remove XML elements
%
% Usage:
% tascar_xml_rm_element( pNode, etype, [cattr1, cvalue1, ...] )
% 
% pNode - parent node of the element to be removed
% cName - name of the element to be deleted
% cattr1 - constraint attribute name
% cvalue1 - constraint attribute value
%

  elem_list = javaMethod('getElementsByTagName',pNode,etype);
  N = javaMethod('getLength',elem_list);
  for k=N:-1:1
    elem = javaMethod('item',elem_list,k-1);
    b_matched = true;
    for katt=1:2:numel(varargin)
      if ~strcmp(javaMethod('getAttribute',elem,varargin{katt}),varargin{katt+1})
	b_matched = false;
      end
    end
    if b_matched
      javaMethod('removeChild',pNode, elem);
    end
  end
