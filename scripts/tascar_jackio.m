function [y,fs] = tascar_jackio( x, csOutputPorts, csInputPorts )
% TASCAR_JACKIO - synchonouos recording/playback via jack
%
% Usage:
% Query jack server settings:
% [fs,bufsize] = tascar_jackio();
%
% Playback of MATLAB vector:
% fs = tascar_jackio( x, csOutputPorts );
%
% Synchronouos playback and recording of MATLAB vector:
% [y,fs] = tascar_jackio( x, csOutputPorts, csInputPorts );
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

  
  [err,msg] = system('LD_LIBRARY_PATH="" jack_bufsize');
  [data,narg] = sscanf(msg,'buffer size = %d sample rate = %d');
  if narg ~= 2
    error('jack_bufsize failed');
  end
  fs = data(2);
  bufsize = data(1);
  if nargin == 0
    y = fs;
    fs = bufsize;
  else
    sInPar = '';
    if nargin == 3
      sNameIn = [tempname(),'.wav'];
      sInPar = ['-o ',sNameIn];
      csInputPorts = portname( csInputPorts, 'input' );
      for k=1:numel(csInputPorts)
	sInPar = [sInPar,' ',csInputPorts{k}];
      end
    end
    if nargin >= 2
      sNameOut = [tempname(),'.wav'];
      if ~isempty(ver('octave'))
	%% this is octave; warn if abs(x) > 1
	if max(abs(x(:))) > 1
	  warning(['Signal clipped: ', ...
		   sNameOut]);
	end
      end
      wavwrite(x,fs,32,sNameOut);
      sCmd = ['tascar_jackio -u ',sNameOut];
      csOutputPorts = portname(csOutputPorts,'output');
      for k=1:numel(csOutputPorts)
	sCmd = [sCmd,' ',csOutputPorts{k}];
      end
      sCmd = [sCmd,' ',sInPar];
      [a,b] = system(['LD_LIBRARY_PATH="" ',sCmd]);
      if ~isempty(b)
	error(b);
      end
      if ~isempty(sInPar)
	y = wavread(sNameIn);
	delete(sNameIn);
      end
    end
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
  