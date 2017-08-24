function tascar_generatedecmatrix( sFileName, iOrder, vAz, sInput )
% tascar_generatedecmatrix - generate 2D HOA max-rE decoder
% settings for matrix module.
%
% Usage:
%
% tascar_generatedecmatrix( sFileName, iOrder, vAz, sInput )
%
% sFileName - output file name
% iOrder - ambisonics order
% vAz - Speaker azimuth in degrees
% sInput - name of hoa2d_fuma output receiver, for connection
  
  nodeDoc = ...
      tascar_xml_doc_new();
  nodeSession = ...
      tascar_xml_add_element( nodeDoc, nodeDoc, 'session' );
  nodeModule = ...
      tascar_xml_add_element( nodeDoc, nodeSession, 'module', [], ...
			      'name', 'matrix', 'id', 'dec' );
  iInputChannels = 2*iOrder+1;
  iOutputChannels = numel(vAz);
  for c=1:iInputChannels
    o = max(0,floor((c+2)/2)-1);
    l = o;
    if( mod(c,2) == 0 )
      l = -o;
    end
    tascar_xml_add_element( nodeDoc, nodeModule, 'input', [], ...
			    'label', sprintf('.%d_%d',o,l), ...
			    'connect', sprintf('%s.%d_%d',sInput,o,l));
  end
  vOrder = [0:iOrder];
  for c=1:iOutputChannels
    az = vAz(c);
    
    % FuMa norm:
    cm = exp(-i*az*pi/180).^vOrder;
    cm(1) = sqrt(0.5);
    cm = 2*cm/iOutputChannels;
    if 1 % maxre
      cm = cm .* cos(vOrder*pi/(2.0*iOrder+2.0));
    end
    m = [imag(cm);real(cm)];
    m = m(2:end);
    tascar_xml_add_element( nodeDoc, nodeModule, 'speaker', [], ...
			    'az', sprintf('%g',az),...
			    'm', sprintf('%g ',m) );
  end
  tascar_xml_save( nodeDoc, sFileName );