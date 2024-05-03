function noisesound( sInput, sOutput, dur, tauenv )
% NOISESOUND - create a random-phase noise signal with input spectrum
%
% Usage:
% noisesound( sInput, sOutput, dur )
%
% sInput : name of input sound file
% sOutput: name of output sound file
% dur    : duration (and fft length) of the output sound file. If dur==0 then the original duration is used.
%
% Only the first channel of the input signal is used. The absolute
% level of the output signal is not matched with the input
% signal. If the output duration is longer than the input signal,
% then the input signal is zero-padded. If the input signal is
% longer than the output duration, then the average power spectrum
% of non-overlapping fragments of length duration is used.
%
% Author: Giso Grimm
% Date: 1/2014, 2024
  ;
  [x,fs] = audioread(sInput);
  if dur > 0
    dur = round(dur*fs);
  else
    dur = size(x,1);
  end
  if nargin < 4
    tauenv = 0;
  end
  x(:,2:end) = [];
  rms_in = sqrt(mean(x.^2));
  x = buffer(x,dur);
  env = mean(abs(hilbert(x)),2);
  X = sqrt(mean(abs(realfft(x)).^2,2));
  X = X .* exp(2*pi*i*rand(size(X)));
  x = realifft(X);
  if tauenv > 0
    rms_out = sqrt(mean(x.^2));
    x = x .* (rms_in./rms_out);
    x = x .* env;
  else
    x = 0.9*x / max(abs(x(:)));
  end
  audiowrite(sOutput,x,fs);
  
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
