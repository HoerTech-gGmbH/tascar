function noisesound( sInput, sOutput, dur )
% NOISESOUND - create a random-phase noise signal with input spectrum
%
% Usage:
% noisesound( sInput, sOutput, dur )
%
% sInput : name of input sound file
% sOutput: name of output sound file
% dur    : duration (and fft length) of the output sound file
%
% Only the first channel of the input signal is used. The absolute
% level of the output signal is not matched with the input
% signal. If the output duration is longer than the input signal,
% then the input signal is zero-padded. If the input signal is
% longer than the output duration, then the average power spectrum
% of non-overlapping fragments of length duration is used.
%
% Author: Giso Grimm
% Date: 1/2014
  ;
  [x,fs] = wavread(sInput);
  dur = round(dur*fs);
  x(:,2:end) = [];
  x = buffer(x,dur);
  X = sqrt(mean(abs(realfft(x)).^2,2));
  X = X .* exp(2*pi*i*rand(size(X)));
  x = realifft(X);
  x = 0.9*x / max(abs(x(:)));
  wavwrite(x,fs,sOutput);
  
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
