function [x,X,phase] = minphase( x )
fftlen = size(x,1);
X = abs(realfft(x));
nbins = size(X,1);
phase = log(max(1e-10,X));
phase(end+1:fftlen,:) = 0;
phase = -imag(hilbert(phase));
X = X .* exp(i*phase(1:nbins,:));
x = realifft(X,fftlen);
