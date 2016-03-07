function tascar_xml_save( doc, ofname )
% tascar_xml_save - save XML document to file
%
% Usage:
% tascar_xml_save( doc, ofname )
%
% doc - document (java object)
% ofname - output file name
  dsource = javaObject('javax.xml.transform.dom.DOMSource',doc);
  transformerFactory = javaMethod('newInstance','javax.xml.transform.TransformerFactory');
  transformer = javaMethod('newTransformer',transformerFactory);
  writer= javaObject('java.io.StringWriter');
  result = javaObject('javax.xml.transform.stream.StreamResult',writer);
  javaMethod('transform',transformer,dsource, result);
  writer.close();
  xml = char(writer.toString());
  % makes each row into a cell
  CellArray = strcat(xml); 
  fid = fopen(ofname,'w');
  for r=1:size(CellArray,1)
    fprintf(fid,'%s\n',CellArray(r,:));
  end
  fclose(fid);
  