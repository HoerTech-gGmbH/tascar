function send_osc_tcp( host, port, addr, varargin )
% send_osc_tcp - send OSC messages via TCP
%
% Usage:
% send_osc( host, port, addr, ... )
%
% Works with strings and floats. Tested on Matlab R2011b and Octave
% 3.8.1.
%
% Author: Giso Grimm
% Date: 6/2012, 5/2014, 2/2015
%
% Depends on 'NetUtil.jar' from http://www.sciss.de/netutil/
%
% Latest version:
% http://mvnrepository.com/artifact/de.sciss/netutil/1.0.0
%
% Installation:
% system('wget http://repo1.maven.org/maven2/de/sciss/netutil/1.0.0/netutil-1.0.0.jar')
% javaaddpath netutil-1.0.0.jar 
   
  dch = javaMethod('open','java.nio.channels.SocketChannel');
  dch.configureBlocking( true );
  target = javaObject('java.net.InetSocketAddress', host, port );
  if ~isempty(varargin)
    jvar = javaArray('java.lang.Object',numel(varargin));
    for k=1:numel(varargin)
      if ischar(varargin{k})
	jvar(k) = javaObject('java.lang.String',varargin{k});
      else
	jvar(k) = javaObject('java.lang.Float',varargin{k});
      end
    end
    osm = javaObject('de.sciss.net.OSCMessage', addr, jvar );
  else
    osm = javaObject('de.sciss.net.OSCMessage', addr );
  end
  buf = javaMethod('allocateDirect','java.nio.ByteBuffer', 8192 );
  osm.encode( buf );
  buf.flip();
  sbufbb = javaMethod('allocateDirect','java.nio.ByteBuffer', 4 );
  javaMethod('putInt',sbufbb,osm.getSize());
  sbufbb.flip();
  dch.connect( target );
  dch.write( sbufbb );
  dch.write( buf );
  dch.close();