function [alpha,freq] = tascar_rflt2alpha( r, d, fs, freq )
% tascar_rflt2alpha - calculate absorption coefficients from reflection filters
%
% Usage:
% [alpha,freq] = tascar_rflt2alpha( r, d, fs [, freq] )
%
% r : reflector 'reflectivity'
% d : reflector 'damping'
% fs : sampling rate / Hz
% freq : absorption coefficient frequencies / Hz
% (optional; default = [125,250,500,1000,2000,4000])
%
% Return values:
% alpha : aborption coefficients
  if nargin < 4
    freq = [125,250,500,1000,2000,4000];
  end
  r = max(min(r,1),eps);
  d = max(min(d,1-eps),-1+eps);
  cw = exp(-i*2*pi*freq/fs);
  H = abs(r*(1-d)./(1-d.*cw));
  alpha = (1-H).^2;
  alpha = alpha(:)';
