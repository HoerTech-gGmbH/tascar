function elem = tascar_xml_set_attribute( elem, attr, value, format )
% tascar_xml_set_attribute - set XML element attribute
%
% Usage:
% elem = tascar_xml_set_attribute( elem, attr, value [, format ] )
% 
% elem - element handle
% attr - attribute name to edit
% value - new value
% format - number format (default: '%g')
%
  if nargin < 4
    format = '%g';
  end
  if ~ischar(value)
    value = sprintf([format,' '],value(:)');
    if ~isempty(value)
      value(end) = [];
    end
  end
  javaMethod('setAttribute',elem,attr,value);
