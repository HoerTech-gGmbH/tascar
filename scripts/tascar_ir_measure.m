function [ir,fs,x,y] = tascar_ir_measure( varargin )
% TASCAR_IR_MEASURE - measure impulse responses via jack
%
% Usage:
% [ir,fs] = tascar_ir_measure( ... )
%
% Use key/value pairs to fine-control the program.
%
% Examples:
% - Show list of valid key/value pairs:
%   tascar_ir_measure help
%
% - Measure impulse response, playback via first hardware channel,
%   recorded from first two hardware inputs:
%   [ir,fs] = tascar_ir_measure( 'output', 1, 'input', 1:2 );
%
% - Measure impulse response from hardware inputs 1, 3 and 7, use
%   the third input (hardware channel 7) as a reference:
%   [ir,fs] = tascar_ir_measure( 'output', 1, 'input', [1,3,7], 'refchannel', 3 );
%
% The test signal is played once before the measurement phase.
%
% Author: Giso Grimm
% Date: 11/2015
  if nargin == 0
    varargin{end+1} = 'help';
    disp(help(mfilename));
  end
  sCfg = struct;
  sHelp = struct;
  sCfg.len = 2^10;
  sHelp.len = 'length of impulse response in samples';
  sCfg.nrep = 1;
  sHelp.nrep = 'number of repetitions in measurement phase';
  sCfg.gain = 0.1;
  sHelp.gain = 'scaling of test signal (peak amplitude re FS)';
  sCfg.output = {};
  sHelp.output = 'output ports';
  sCfg.input = {};
  sHelp.input = 'input ports';
  sCfg.refchannel = [];
  sHelp.refchannel = 'use a recorded channel as reference signal';
  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
  if isempty(sCfg)
    return;
  end
  if nargin < 6
    refchannel = [];
  end
  if ischar(sCfg.output)
    nChannels = 1;
  else
    nChannels = numel(sCfg.output);
  end
  x = create_testsig( sCfg.len, sCfg.gain );
  [y,fs,bufsize,load,xruns] = ...
      tascar_jackio( repmat(x,[sCfg.nrep+1,nChannels]), ...
		     'output',sCfg.output, ...
		     'input',sCfg.input );
  if xruns > 0
    warning(sprintf('%d xruns!',xruns));
  end
  y(1:sCfg.len,:) = [];
  y = reshape( y, [sCfg.len, sCfg.nrep, size(y,2)] );
  ymean = squeeze(mean( y, 2 ));
  if isempty(sCfg.refchannel)
    X = realfft(x);
    Y = realfft(ymean);
  else
    X = realfft(ymean(:,sCfg.refchannel));
    Y = realfft(ymean(:,setdiff(1:size(ymean,2),sCfg.refchannel)));
  end
  Y = Y ./ repmat(X,[1,size(Y,2)]);
  ir = realifft(Y,sCfg.len);

function x = create_testsig( len, gain )
  x = (rand(len,1)-0.5)*gain;
  X = realfft(x);
  vF = [1:size(X,1)]';
  vF = vF / max(vF);
  X = exp(-len*i*(2*pi)*vF.^2)./max(0.1,vF.^0.5);
  x = realifft(X,len);
  x = 0.5*x/max(abs(x))*gain;
