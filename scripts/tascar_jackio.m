function [y,fs,bufsize,load,xruns,sCfg] = tascar_jackio( x, varargin )
% TASCAR_JACKIO - synchonouos recording/playback via jack
%
% Usage:
% [y,fs,bufsize,load,xruns,sCfg] = tascar_jackio( x, ... );
%
% Examples:
% 
% Display list of valid key/value pairs:
% tascar_jackio help
%
% Playback of MATLAB matrix x on hardware outputs (one column per channel):
% tascar_jackio( x );
%
% Playback of MATLAB data via user defined ports:
% tascar_jackio( x, 'output', csOutputPorts );
%
% Synchronouos playback and recording of MATLAB vector:
% [y,fs,bufsize] = tascar_jackio( x, 'output', csOutputPorts, 'input', csInputPorts );
%
% Synchronouos playback and recording, with jack transport:
% [y,fs,bufsize] = tascar_jackio( x, 'input', csInputPorts, 'starttime', transportStart );
%
% Synchronouos playback and recording, in freewheeling mode:
% [y,fs,bufsize] = tascar_jackio( x, 'input', csInputPorts, 'freewheeling', true );
%
% If csOutputPorts and csInputPorts are a single string, a single
% port name is used. If they are a cell string array, the number of
% channels in x has to match the number of elements in
% csOutputPorts, and the number of channels in y will match the
% number of channels in csInputPorts. If the argument is a numeric
% vector, then the physical ports (starting from 1) are used.
%
% Author: Giso Grimm
% Date: 11/2013

  if nargin == 0
    x = [];
    varargin{end+1} = 'help';
    disp(help(mfilename));
  end
  if (nargin == 1) && ischar(x) && strcmp(x,'help')
    varargin{end+1} = 'help';
    x = [];
  end
  sCfg = struct;
  sHelp = struct;
  sCfg.input = {};
  sHelp.input = ['input port names, single string, cell string or' ...
		 ' physical port numbers'];
  sCfg.output = [1:size(x,2)];
  sHelp.output = ['output port names, single string, cell string or' ...
		  ' physical port numbers'];
  sCfg.starttime = [];
  sHelp.starttime = ['if not empty use jack transport and start from' ...
		     ' this time'];
  sCfg.wait = false;
  sHelp.wait = ['instead of re-locating transport wait for start-time'];
  sCfg.freewheeling = false;
  sHelp.freewheeling = ['switch to freewheeling mode'];
  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
  if isempty(sCfg)
    return;
  end
  [err,msg] = system('LD_LIBRARY_PATH="" tascar_jackpar');
  [data,narg] = sscanf(msg,'%d %d');
  if narg ~= 2
    error('tascar_jackpar failed');
  end
  y = [];
  fs = data(2);
  bufsize = data(1);
  %if nargin < 2
  %  csOutputPorts = [1:size(x,2)];
  %end
  %if nargin < 3
  %  csInputPorts = {};
  %end
  %if nargin < 4
  %  transportStart = [];
  %end
  sInPar = '';
  sCmd = 'tascar_jackio';
  if ~isempty(sCfg.starttime)
    sCmd = [sCmd,sprintf(' -s %g',sCfg.starttime)];
  end
  if sCfg.freewheeling
    sCmd = [sCmd,' -f'];
  end
  if sCfg.wait
      sCmd = [sCmd,' -w'];
  end
  sStatname = tempname();
  sCmd = [sCmd,' -t ',sStatname];
  if ~isempty(sCfg.input)
    sNameIn = [tempname(),'.wav'];
    sInPar = ['-o ',sNameIn];
    sCfg.input = portname( sCfg.input, 'input' );
    for k=1:numel(sCfg.input)
      sInPar = [sInPar,' ',sCfg.input{k}];
    end
  end
  if ~isempty(sCfg.output)
    sNameOut = [tempname(),'.wav'];
    if ~isempty(ver('octave'))
      %% this is octave; warn if abs(x) > 1
      if max(abs(x(:))) > 1
	warning(['Signal clipped: ',sNameOut]);
      end
    end
    audiowrite(sNameOut,x,fs,'BitsPerSample',32);
    sCmd = [sCmd,' -u ',sNameOut];
    sCfg.output = portname(sCfg.output,'output');
    for k=1:numel(sCfg.output)
      sCmd = [sCmd,' ',sCfg.output{k}];
    end
  end
  sCmd = [sCmd,' ',sInPar];
  %disp(sCmd)
  [a,b] = system(['LD_LIBRARY_PATH="" ',sCmd]);
  if ~isempty(b)
    error(b);
  end
  if ~isempty(sInPar)
    y = audioread(sNameIn);
    delete(sNameIn);
  end
  fh = fopen(sStatname,'r');
  [data,cnt] = fscanf(fh,'cpuload %g xruns %g');
  fclose(fh);
  load = -1;
  xruns = -1;
  if cnt == 2
    load = data(1);
    xruns = data(2);
  end
  %sStat = textread(sStatname);
  %load = sStat(2);
  %xruns = sStat(3);
  delete(sStatname);
  if nargout == 0
    clear('y');
  end
  
function csPort = portname( csPort, mode )
  if ischar(csPort)
    csPort = {csPort};
  else
    if isnumeric(csPort)
      csNew = {};
      for k=1:numel(csPort)
	if strcmp( mode, 'input' )
	  csNew{k} = sprintf('system:capture_%d',csPort(k));
	else
	  csNew{k} = sprintf('system:playback_%d',csPort(k));
	end
      end
      csPort = csNew;
    end
  end
  
