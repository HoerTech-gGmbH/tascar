function [irs,fs,x,y] = getirs( spk, len, nrep, sTag, gain, sIn )
% GETIRS - record impulse responses
%
% Usage:
% [irs,x,y] = getirs( spk, len, nrep, sTag, gain );
%
% spk : vector with output speaker channels or cell string array
%       with output jack ports
% len : length in samples
% nrep : number of repetitions
% sTag : tag to be used in output file
% gain : linear gain of output signal (optional, default: 0.02)
% sIn : input channel name (default: "system:capture_1")
%
% irs : impulse response matrix
% x : output test signal
% y : averaged input signal
%
% Author: Giso Grimm
% 3/2012

  if nargin < 5
    gain = 0.02;
  end
  if nargin < 6
    sIn = 'system:capture_1';
  end
  x = create_noise( len, nrep+1, gain );
  irs = zeros(len,numel(spk));
  for kSpk=1:numel(spk)
    sFile = sprintf('rec_%s_%d_%d_%d.wav',sTag,spk(kSpk),len,nrep);
    system(sprintf(['LD_LIBRARY_PATH="" tascar_jackio wnoise.wav -o %s ' ...
		    'system:playback_%d %s'],sFile,spk(kSpk),sIn));
    [y,fs] = wavread(sFile);
    y(1:len,:) = [];
    y = reshape( y, [len, nrep, size(y,2)] );
    y = squeeze(mean( y, 2 ));
    %addpath('tools');
    X = realfft(x);
    Y = realfft(y);
    Y = Y ./ repmat(X,[1,size(Y,2)]);
    irs(:,kSpk) = realifft(Y);
  end
  wavwrite(irs,fs,32,sprintf('irs_%s_%d_%d.wav',sTag,len, ...
			     nrep));
  
function x = create_noise( len, nrep, gain )
  x = (rand(len,1)-0.5)*gain;
  X = realfft(x);
  vF = [1:size(X,1)]';
  vF = vF / max(vF);
  X(:) = exp(-len*i*(2*pi)*vF.^2)./max(0.1,vF.^0.5);
  x = realifft(X);
  x = 0.5*x/max(abs(x))*gain;
  wavwrite(repmat(x,[nrep,1]),48000,16,'wnoise.wav');

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
