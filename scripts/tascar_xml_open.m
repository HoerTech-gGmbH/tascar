function doc = tascar_xml_open( ifname )
% tascar_xml_open - open an xml file
%
% Usage:
% doc = tascar_xml_open( ifname );
%
% ifname - input file name
% doc - document (java object)
  
  factory = javaMethod('newInstance', ...
		       'javax.xml.parsers.DocumentBuilderFactory');
  docBuilder = javaMethod('newDocumentBuilder',factory);
  % Open xml file:
  [sdir,name,ext] = fileparts(ifname);
  if isempty(sdir) || (sdir(1) ~= '/')
    ifname = [pwd(),filesep(),ifname];
  end
  doc = javaMethod('parse',docBuilder,ifname);
  