function elem = tascar_xml_set_attribute( elem, attr, value )
% tascar_xml_set_attribute - set XML element attribute
%
% Usage:
% elem = tascar_xml_set_attribute( elem, attr, value )
% 
% elem - element handle
% attr - attribute name to edit
% value - new value
%
  if ~ischar(value)
    value = sprintf('%g ',value(:)');
  end
  javaMethod('setAttribute',elem,attr,value);
