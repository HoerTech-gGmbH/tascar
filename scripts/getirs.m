function [irs,fs,x,y] = getirs( spk, len, nrep, sTag, gain, sIn )
% GETIRS - record impulse responses
%
% Usage:
% [irs,fs,x,y] = getirs( spk, len, nrep, sTag, gain, sIn );
%
% spk  : vector with hardware output channel numbers or cell string 
%        array with output jack port names
% len  : length in samples
% nrep : number of repetitions
% sTag : tag to be used in output file
% gain : linear gain of output signal (optional, default: 0.02)
% sIn  : input channel name (default: "system:capture_1")
%
% irs  : impulse response matrix
% fs   : sampling rate of jack
% x    : output test signal
% y    : averaged input signal
%
% Author: Giso Grimm
% 3/2012

  if nargin < 5
    gain = 0.02;
  end
  if nargin < 6
    sIn = 'system:capture_1';
  end
  Nsrc = 1;
  if iscell(sIn)
    Nsrc = numel(sIn);
    sTmp = '';
    for k=1:Nsrc
      sTmp = sprintf('%s %s',sTmp,sIn{k});
    end
    if Nsrc > 0
      sTmp(1) = [];
    end
    sIn = sTmp;
  end
  Nsink = numel(spk);
  x = create_noise( len, gain );
  X = realfft(x);
  irs = zeros(len,Nsink*Nsrc);
  for kSpk=1:numel(spk)
    if iscell(spk(kSpk))
        speakerName = spk{kSpk};
    else
        speakerName = sprintf('system:playback_%d', spk(kSpk));
    end
    [y,fs] = tascar_jackio(repmat(x,[nrep+1,1]),speakerName,sIn);
    y(1:len,:) = [];
    y = reshape( y, [len, nrep, size(y,2)] );
    y = squeeze(mean( y, 2 ));
    %addpath('tools');
    Y = realfft(y);
    Y = Y ./ repmat(X,[1,size(Y,2)]);
    irs(:,(kSpk-1)*Nsrc+[1:Nsrc]) = realifft(Y);
  end
  if ~isempty(ver('octave'))
    %% this is octave; warn if abs(x) > 1
    if max(abs(irs(:))) > 1
      sNameOut = sprintf('irs_%s_%d_%d.wav',sTag,len,nrep);
      warning(['Signal clipped: ',sNameOut]);
    end
  end
  wavwrite(irs,fs,32,sprintf('irs_%s_%d_%d.wav',sTag,len, ...
			     nrep));
  
function x = create_noise( len, gain )
  x = (rand(len,1)-0.5)*gain;
  X = realfft(x);
  vF = [1:size(X,1)]';
  vF = vF / max(vF);
  X = exp(-len*i*(2*pi)*vF.^2)./max(0.1,vF.^0.5);
  x = realifft(X);
  x = 0.5*x/max(abs(x))*gain;

function y = realfft( x )
% REALFFT - FFT transform of pure real data
%
% Usage: y = realfft( x )
%
% Returns positive frequencies of fft(x), assuming that x is pure
% real data. Each column of x is transformed separately.
  ;
  fftlen = size(x,1);
  
  y = fft(x);
  y = y([1:floor(fftlen/2)+1],:);

function x = realifft( y )
% REALIFFT - inverse FFT of positive frequencies in y
%
% Usage: x = realifft( y )
%
% Returns inverse FFT or half-complex spectrum. Each column of y is
% taken as a spectrum.
  ;
  channels = size(y,2);
  nbins = size(y,1);
  x = zeros(2*(nbins-1),channels);
  for ch=1:channels
    ytmp = y(:,ch);
    ytmp(1) = real(ytmp(1));
    ytmp(nbins) = real(ytmp(nbins));
    ytmp2 = [ytmp; conj(ytmp(nbins-1:-1:2))];
    x(:,ch) = real(ifft(ytmp2));
  end
