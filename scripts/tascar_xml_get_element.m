function h = tascar_xml_get_element( doc, etype, varargin )
% Get a handle to the element that you want
%
% Usage:
% h = tascar_xml_get_element( doc, etype [, cattr1, cvalue1, ...] )
% 
% doc - document (java object) or element
% etype - element name
% cattr1 - constraint attribute name
% cvalue1 - constraint attribute value
%
    elem_list = javaMethod('getElementsByTagName',doc,etype);
    N = javaMethod('getLength',elem_list);
    h = {};
    for k=1:N
        elem = javaMethod('item',elem_list,k-1);
        b_matched = true;
        for k=1:2:numel(varargin)
            if ~strcmp(javaMethod('getAttribute',elem,varargin{k}),varargin{k+1})
                b_matched = false;
            end
        end
        if b_matched
            h{end+1} = elem;
        end
    end
