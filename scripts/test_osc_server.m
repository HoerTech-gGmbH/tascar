if isempty(cell2mat(strfind(javaclasspath(),'netutil')))
  javaaddpath([fileparts(fullfile(which(mfilename))),...
               filesep,'netutil-1.0.0.jar']);
end
%codec = javaObject('de.sciss.net.OSCPacketCodec');
proto = javaObject('java.lang.String', 'udp' );
%proto = javaObject('de.sciss.net.OSCServer.UDP' );
%proto = javaMethod('OSCChannel.UDP','de.sciss.net.OSCServer' );

%interface = javaMethod('de.sciss.net.OSCChannel.UDP');
oscsrv = javaMethod('newUsing','de.sciss.net.OSCServer', proto, 8001 );
