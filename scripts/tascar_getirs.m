function [irs,fs] = tascar_getirs( len, nrep, gain, csOutputPorts, csInputPorts, refchannel )
% TASCAR_GETIRS - record impulse responses via jack
%
% Usage:
% [irs,fs] = tascar_getirs( len, nrep, gain, csOutputPorts, csInputPorts [, refchannel ] )
%
% len            : length in samples
%
% nrep           : number of repetitions for averaging
%
% gain           : scaling of test signal (peak amplitude re FS)
%
% csOutputPorts, 
% csInputPorts   : port names (see tascar_jackio for details)
%
% refchannel     : use a recorded channel as reference signal (optional)
%
% Author: Giso Grimm
% Date: 11/2013
  if nargin < 6
    refchannel = [];
  end
  if ischar(csOutputPorts)
    nChannels = 1;
  else
    nChannels = numel(csOutputPorts);
  end
  x = create_testsig( len, gain );
  [y,fs] = tascar_jackio( repmat(x,[nrep+1,nChannels]), ...
			  csOutputPorts, ...
			  csInputPorts );
  y(1:len,:) = [];
  y = reshape( y, [len, nrep, size(y,2)] );
  y = squeeze(mean( y, 2 ));
  if isempty(refchannel)
    X = realfft(x);
    Y = realfft(y);
  else
    X = realfft(y(:,refchannel));
    Y = realfft(y(:,setdiff(1:size(y,2),refchannel)));
  end
  Y = Y ./ repmat(X,[1,size(Y,2)]);
  irs = realifft(Y);


function x = create_testsig( len, gain )
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
