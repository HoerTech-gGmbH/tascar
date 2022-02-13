function [r,l1,l2] = drr( irs, fs, x, tw, s )
% DRR - calculate direct-to-reverberant ratio
%
% Usage:
% r = drr( irs, fs [, x [, tw, s] ] )
%
% Input:
%   irs : impulse response 
%
%   fs  : sampling rate
%
%   x : test signal (optional; if empty or not provided then white
%       noise is used)
%
%   tw, s: integration time and slope for direct sound (after
%          Bronkhorst, 1999); optional; defaults: tw=0.0061 [s],
%          s=400 [s^-1]
%
% Output:
%   r   : direct to reverberant ratio in dB (Hallmass)
%
% Author: Giso Grimm, 2015
  r = zeros([1,size(irs,2)]);
  l1 = zeros([1,size(irs,2)]);
  l2 = zeros([1,size(irs,2)]);
  if nargin < 3
    x = [];
  end
  if isempty(x)
    rand('seed',1);
    x = rand(4*fs,1)-0.5;
  end
  if nargin < 4
    tw = 0.0061;
  end
  if nargin < 5
    s = 400;
  end
  vT = [1:size(irs,1)]'/fs;
  tw1 = tw-pi/(2*s);
  tw2 = tw+pi/(2*s);
  idx = irs_firstpeak(irs);
  dt = round(fs*0.001);
  for kch=1:size(irs,2)
    t = vT-idx(kch)/fs;
    w = zeros(size(t));
    tidx1 = find(t<=tw1);
    tidxs = find((tw1<t) & (t<tw2));
    tidx0 = find(tw2<=t);
    w(tidx1) = 1;
    w(tidxs) = 0.5-0.5*sin(s*(t(tidxs)-tw));
    w(tidx0) = 0;
    h_d = irs(:,kch) .* w;
    h_r = irs(:,kch) .* (1-w);
    y_dir = fftfilt( h_d, x );
    y_rev = fftfilt( h_r, x );
    l1(kch) = 10*log10(mean(y_dir.^2));
    l2(kch) = 10*log10(mean(y_rev.^2));
    r(kch) = l1(kch) - l2(kch);
  end
