function pinknoise( fname, duration, fs )
% pinknoise( fname, duration )

if nargin < 3
  fs = 44100;
end
if nargin < 2
  duration = 1;
end
fmin = 100;
N = round(duration*fs);
Nbins = floor(N/2)+1;
vF = ([1:Nbins]'-1)/fs;
H = fmin./max(vF,fmin) .* exp(i*2*pi*rand(Nbins,1));
y = realifft(H);
y = 0.5/max(abs(y(:)))*y;
audiowrite(fname,y,fs);
