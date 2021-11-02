function [y,sCfg] = tascar_renderscene( x, session, varargin )
% TASCAR_renderscene - offline rendering of a scene
%
% Usage:
% [y,sCfg] = tascar_renderscene( x, session ... );
%
% Examples:
% help tascar_renderscene
% tascar_renderscene help
%
% y = tascar_renderscene( zeros(44100,1), '/usr/share/tascar/examples/example_basic.tsc');
%
% Author: Giso Grimm
% Date: 1/2020

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
  sCfg.fs = 44100;
  sHelp.fs = 'sampling rate in Hz';
  sCfg.scene = '';
  sHelp.scene = 'scene name, or empty for first scene of session file';
  sCfg.starttime = [];
  sHelp.starttime = ['if not empty use jack transport and start from' ...
		     ' this time'];
  sCfg.dynamic = false;
  sHelp.dynamic = 'dynamic rendering';
  sCfg.fragsize = 1024;
  sHelp.fragsize = 'fragment size in samples';
  sCfg.ismmin = 0;
  sHelp.ismmin = 'minimum ISM order';
  sCfg.ismmax = 1;
  sHelp.ismmax = 'maximum ISM order';
  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
  if isempty(sCfg)
    return;
  end
  sInPar = '';
  sCmd = 'tascar_renderfile';
  sCmd = [sCmd,' ',session];
  if ~isempty(sCfg.scene)
    sCmd = [sCmd,sprintf(' -s %s',sCfg.scene)];
  end
  if ~isempty(sCfg.starttime)
    sCmd = [sCmd,sprintf(' -t %g',sCfg.starttime)];
  end
  if ~isempty(sCfg.ismmin)
    sCmd = [sCmd,sprintf(' --ismmin=%d',sCfg.ismmin)];
  end
  if ~isempty(sCfg.ismmax)
    sCmd = [sCmd,sprintf(' --ismmax=%d',sCfg.ismmax)];
  end
  if sCfg.dynamic
    sCmd = [sCmd,' -d'];
  end
  sNameIn = [tempname(),'.wav'];
  sCmd = [sCmd,' -i ',sNameIn];
  if ~isempty(ver('octave'))
    %% this is octave; warn if abs(x) > 1
    if max(abs(x(:))) > 1
      warning(['Signal clipped: ',sNameIn]);
    end
  end
  audiowrite(sNameIn,x,sCfg.fs,'BitsPerSample',32);
  sNameOut = [tempname(),'.wav'];
  sCmd = [sCmd,' -o ',sNameOut];
  %disp(sCmd)
  [a,b] = system(['LD_LIBRARY_PATH="" ',sCmd]);
  if ~isempty(b)
    error(b);
  end
  y = audioread(sNameOut);
  delete(sNameIn);
  delete(sNameOut);
  if nargout == 0
    clear('y');
  end
