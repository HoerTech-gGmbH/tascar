function cNode = tascar_xml_add_element( doc, pNode, cName, sContent, varargin ) 
% tascar_xml_add_element - add an element to a node
%
% Usage:
% cNode = tascar_xml_add_element( doc, pNode, cName [, sContent [, attr, value, ... ] ]) 
%
% doc - document (java object)
% pNode - parent node where to add the new element
% cName - name of the new element
% sContent - optional content, or empty matrix
% cNode - new element
  ;
  if nargin < 4
    sContent = [];
  end
  %create a new element:
  if mod(length(varargin),2)
    error('For each attribute there must be exactly one corresponding value')
  end
  cNode = javaMethod( 'createElement', doc, cName );
  %set attributes of the new element:
  for k=1:2:length(varargin)  
    if ~ischar(varargin{k})
      warning('character type required for attribute name');
    end
    if ~ischar(varargin{k+1})
      warning('character type required for attribute value');
    end
    javaMethod('setAttribute',cNode,varargin{k},varargin{k+1});
  end
  % specify the parent node:
  javaMethod('appendChild',pNode,cNode);
  if ~isempty(sContent)
    javaMethod('appendChild',cNode,javaMethod('createTextNode',doc, sContent));
  end
