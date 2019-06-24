function x = realifft( y, fftlen )
% REALIFFT - inverse FFT of positive frequencies in y
%
% Usage: x = realifft( y [, fftlen] )
%
% Returns inverse FFT or half-complex spectrum. Each column of y is
% taken as a spectrum.
  ;
  channels = size(y,2);
  if nargin < 2
      fftlen = 2*(size(y,1)-1);
  end
  nbins = floor(fftlen/2+1);
  x = zeros(fftlen,channels);
  for ch=1:channels
    ytmp = y(:,ch);
    if iseven(fftlen)
        ytmp(1) = real(ytmp(1));
        ytmp(nbins) = real(ytmp(nbins));
        ytmp2 = [ytmp; conj(ytmp(nbins-1:-1:2))];
    else
        ytmp(1) = real(ytmp(1));
        ytmp2 = [ytmp; conj(ytmp(nbins:-1:2))];
    end
    x(:,ch) = real(ifft(ytmp2));
  end

function b = iseven( n )
    b = (mod(n,2)==0);