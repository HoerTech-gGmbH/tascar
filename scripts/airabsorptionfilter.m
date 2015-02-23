function [B,A] = airabsorptionfilter( d, fs, c, alpha )
  if nargin < 4
    alpha = 7782;
  end
  if nargin < 3
    c = 340;
  end
  if nargin < 2
    fs = 44100;
  end
  B = exp(-d*fs/(c*alpha));
  A = [1,-(1-B)];
