function val = tascar_xml_get_attribute( elem, attr )
% tascar_xml_get_attribute - return attribute value of an element
%
% Usage:
% val = tascar_xml_get_attribute( elem, attr );
% 
% 'elem' can be an XML element or a cell array of elements.
if iscell(elem)
    val = {};
    for k=1:numel(elem)
        val{k} = javaMethod( 'getAttribute', elem{k}, attr );
    end
else
    val = javaMethod( 'getAttribute', elem, attr );
end