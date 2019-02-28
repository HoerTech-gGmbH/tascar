function d = groupdelay( ir )
% groupdelay - calculate group delay of impulse response
%
% d = groupdelay( ir )
%
% ir : impulse response
% d  : group delay in samples
  H = realfft(ir);
  d = -(size(H,1)-1)*angle(H(2:end,:).*conj(H(1:end-1,:)))/pi;
  d(end+1,:) = d(end,:);

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
