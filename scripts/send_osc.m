function send_osc( host, port, addr, varargin )
% send_osc - send OSC messages via UDP
%
% Usage:
% send_osc( host, port, addr, ... )
% send_osc( handle, addr, ... )
%
% where 'handle' is the return value of tascar_ctl('load',...)
%
% Works with strings and floats. Tested on Matlab R2011b and Octave
% 3.8.1.
%
% Author: Giso Grimm
% Date: 6/2012, 5/2014, 8/2019
%
% Depends on 'NetUtil.jar' from http://www.sciss.de/netutil/
%
% Latest version:
% http://mvnrepository.com/artifact/de.sciss/netutil/1.0.0
%
% Installation:
% system('wget http://repo1.maven.org/maven2/de/sciss/netutil/1.0.0/netutil-1.0.0.jar')
% javaaddpath netutil-1.0.0.jar 
    
    if isstruct( host )
        if nargin > 2
            varargin = [{addr},varargin];
        end
        addr = port;
        port = host.port;
        host = host.host;
    end
    if isempty(cell2mat(strfind(javaclasspath(),'netutil')))
        javaaddpath([fileparts(fullfile(which(mfilename))),...
                     filesep,'netutil-1.0.0.jar']);
    end
    dch = javaMethod('open','java.nio.channels.DatagramChannel');
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
    dch.send( buf, target );
    dch.close();
    