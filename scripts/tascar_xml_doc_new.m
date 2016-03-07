function doc = tascar_xml_doc_new
% tascar_xml_doc_new - create new xml document
  factory = javaMethod('newInstance', ...
		       'javax.xml.parsers.DocumentBuilderFactory');
  docBuilder = javaMethod('newDocumentBuilder',factory);
  doc = javaMethod('newDocument',docBuilder);
