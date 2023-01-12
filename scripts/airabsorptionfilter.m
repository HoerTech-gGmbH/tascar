function [B,A] = airabsorptionfilter( d, fs, c, alpha )
% airabsorptionfilter - calculate distance-dependent air absorption filter coefficients
%
% d : distance in m
% fs: sampling rate in Hz (default: 44100)
% c : speed of sound in m/s (default: 340)
% alpha: scaling variable, dimensionless (default: 7782)
%
% Example:
% Create filtered white noise in 100m distance:
% x = rand(44100,1)-0.5;
% [B,A] = airabsorptionfilter( 100 );
% y = filter(B, A, x);

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
